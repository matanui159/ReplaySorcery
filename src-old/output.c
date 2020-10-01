/*
 * Copyright (C) 2020  Joshua Minter
 * Copyright (C) 2020  Patryk Seregiet
 *
 * This file is part of ReplaySorcery.
 *
 * ReplaySorcery is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ReplaySorcery is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ReplaySorcery.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "output.h"
#include "compress.h"
#include "util/log.h"
#include "util/memory.h"
#include "util/path.h"
#include <errno.h>
#include <minimp4.h>
#include <x264.h>

#define OUTPUT_TIMEBASE 90000
#define OUTPUT_PROGRESS 10

static int outputWrite(int64_t offset, const void *buffer, size_t size, void *data) {
   RSOutput *output = data;
   long seek = (long)offset;
   if (ftell(output->file) != seek) {
      fseek(output->file, seek, SEEK_SET);
   }
   fwrite(buffer, size, 1, output->file);
   return 0;
}

static void outputNals(mp4_h26x_writer_t *track, x264_nal_t *nals, int nalCount,
                       unsigned duration) {
   for (int i = 0; i < nalCount; ++i) {
      mp4_h26x_write_nal(track, nals[i].p_payload, nals[i].i_payload, duration);
   }
}

static void outputCommand(const char *command) {
   if (*command == '\0') {
      // Ignore empty strings
      return;
   }
   rsLog("Running command: %s", command);
   int ret = system(command);
   if (ret != 0) {
      rsLog("Warning: command returned non-zero exit code (%i): %s", ret, command);
   }
}

static void *outputThread(void *data) {
   RSOutput *output = data;
   outputCommand(output->config->preOutputCommand);

   RSDecompress decompress;
   rsDecompressCreate(&decompress);
   RSBuffer *buffer = &output->frames;

   x264_param_t params;
   x264_param_default(&params);
   x264_param_default_preset(&params, output->config->outputX264Preset, NULL);
   x264_param_apply_profile(&params, "high");
   params.i_width = output->config->width;
   params.i_height = output->config->height;
   params.i_csp = X264_CSP_I420;
   params.i_frame_total = (int)output->frameCount;
   params.i_timebase_num = 1;
   params.i_timebase_den = OUTPUT_TIMEBASE;
   params.i_fps_num = (uint32_t)output->config->framerate;
   params.i_fps_den = 1;
   params.b_repeat_headers = false;
   x264_t *x264 = x264_encoder_open(&params);

   MP4E_mux_t *muxer = MP4E_open(false, false, output, outputWrite);
   if (muxer == NULL) {
      rsError("Failed to create MP4 muxer");
   }
   mp4_h26x_writer_t track;
   mp4_h26x_write_init(&track, muxer, output->config->width, output->config->height,
                       false);

   // setup for audio encoding
   RSAudioEncoder audioenc;
   rsAudioEncoderCreate(&audioenc, output->config);
   MP4E_track_t tr;
   tr.track_media_kind = e_audio;
   tr.language[0] = 'u';
   tr.language[1] = 'n';
   tr.language[2] = 'd';
   tr.language[3] = 0;
   tr.object_type_indication = MP4_OBJECT_TYPE_AUDIO_ISO_IEC_14496_3;
   tr.time_scale = OUTPUT_TIMEBASE;
   tr.default_duration = 0;
   tr.u.a.channelcount = (unsigned)output->config->audioChannels;
   int audio_track_id = MP4E_add_track(muxer, &tr);
   MP4E_set_dsi(muxer, audio_track_id, audioenc.aac_info.confBuf,
                (int)audioenc.aac_info.confSize);
   int64_t ts = 0;
   int64_t ats = 0;
   int samplesCount = 0;

   RSFrame frame, yFrame, uFrame, vFrame;
   rsFrameCreate(&frame, (size_t)output->config->width, (size_t)output->config->height,
                 3);
   rsFrameCreate(&yFrame, frame.width, frame.height, 1);
   rsFrameCreate(&uFrame, frame.width / 2, frame.height / 2, 1);
   rsFrameCreate(&vFrame, frame.width / 2, frame.height / 2, 1);
   x264_picture_t inPic, outPic;
   x264_picture_init(&inPic);
   inPic.img.i_csp = X264_CSP_I420;
   inPic.img.i_plane = 3;
   inPic.img.plane[0] = yFrame.data;
   inPic.img.i_stride[0] = (int)yFrame.strideY;
   inPic.img.plane[1] = uFrame.data;
   inPic.img.i_stride[1] = (int)uFrame.strideY;
   inPic.img.plane[2] = vFrame.data;
   inPic.img.i_stride[2] = (int)vFrame.strideY;
   unsigned duration = (unsigned)(OUTPUT_TIMEBASE / output->config->framerate);

   x264_nal_t *nals;
   int nalCount;
   x264_encoder_headers(x264, &nals, &nalCount);
   outputNals(&track, nals, nalCount, 0);

   for (size_t i = 0; i < output->frameCount; ++i) {
      if (i % OUTPUT_PROGRESS == 0) {
         rsLog("Output progress: %zu/%zu", i, output->frameCount);
      }
      rsDecompress(&decompress, &frame, buffer);
      buffer = NULL;
      rsFrameConvertI420(&frame, &yFrame, &uFrame, &vFrame);
      inPic.i_pts = (int64_t)(duration * i);
      x264_encoder_encode(x264, &nals, &nalCount, &inPic, &outPic);
      outputNals(&track, nals, nalCount, duration);
      if (output->rawSamples == NULL) {
         continue;
      }
      ts += (OUTPUT_TIMEBASE / output->config->framerate) * output->config->audioChannels;
      while (ats < ts) {
         uint8_t buf[AAC_OUTPUT_BUFFER_SIZE];
         int numBytes = 0;
         int numSamples = 0;
         rsAudioEncoderEncode(&audioenc, output->rawSamples, buf, &numBytes, &numSamples);
         output->rawSamples += audioenc.frameSize;
         samplesCount += numSamples;
         ats = (int64_t)samplesCount * OUTPUT_TIMEBASE / output->config->audioSamplerate;
         if (MP4E_STATUS_OK !=
             MP4E_put_sample(muxer, audio_track_id, buf, numBytes,
                             (numSamples / output->config->audioChannels) *
                                 OUTPUT_TIMEBASE / output->config->audioSamplerate,
                             MP4E_SAMPLE_RANDOM_ACCESS)) {
            rsError("MP4E_put_sample failed\n");
         }
      }
   }

   while (x264_encoder_delayed_frames(x264) > 0) {
      x264_encoder_encode(x264, &nals, &nalCount, NULL, &outPic);
      outputNals(&track, nals, nalCount, duration);
   }
   rsLog("Output progress: %zu/%zu", output->frameCount, output->frameCount);

   x264_encoder_close(x264);
   mp4_h26x_write_close(&track);
   MP4E_close(muxer);
   fclose(output->file);

   rsFrameDestroy(&vFrame);
   rsFrameDestroy(&uFrame);
   rsFrameDestroy(&yFrame);
   rsFrameDestroy(&frame);
   rsDecompressDestroy(&decompress);
   rsBufferDestroy(&output->frames);

   outputCommand(output->config->postOutputCommand);
   return NULL;
}

void rsOutputCreate(RSOutput *output, const RSConfig *config, RSAudio *audio) {
   output->config = config;
   RSBuffer path;
   rsBufferCreate(&path);
   rsPathAppendDated(&path, config->outputFile);
   output->file = fopen(path.data, "wb");
   if (output->file == NULL) {
      rsError("Failed to open output file '%s': %s", path.data, strerror(errno));
   }
   rsBufferDestroy(&path);
   if (audio->data.data != NULL) {
      output->rawSamples = rsMemoryCreate(audio->data.size);
      output->rawSamplesOriginal = output->rawSamples;
   }
}

void rsOutputDestroy(RSOutput *output) {
   if (output->thread != 0) {
      pthread_join(output->thread, NULL);
   }
   if (output->rawSamplesOriginal != NULL) {
      rsMemoryDestroy(output->rawSamplesOriginal);
   }
   rsMemoryClear(output, sizeof(RSOutput));
}

void rsOutput(RSOutput *output, const RSBufferCircle *frames, RSAudio *audio) {
   rsBufferCreate(&output->frames);
   rsBufferCircleExtract(frames, &output->frames);
   output->frameCount = frames->size;
   if (output->rawSamples != NULL) {
      rsAudioGetSamples(audio, output->rawSamples, frames->size);
   }
   pthread_create(&output->thread, NULL, outputThread, output);
}
