/*
 * Copyright (C) 2020  Joshua Minter
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
#include "config.h"
#include "util.h"
#include <libavutil/avutil.h>
#include <libavutil/bprint.h>
#include <time.h>

int rsOutputCreate(RSOutput *output, int64_t startTime) {
   int ret;
   rsClear(output, sizeof(RSOutput));
   output->startTime = startTime;
   AVBPrint buffer;
   av_bprint_init(&buffer, 0, AV_BPRINT_SIZE_UNLIMITED);
   char *path = NULL;
   const char *outputFile = rsConfig.outputFile;
   if (outputFile[0] == '~') {
      const char *home = getenv("HOME");
      if (home == NULL) {
         av_log(NULL, AV_LOG_ERROR, "Failed to get $HOME variable\n");
         ret = AVERROR_EXTERNAL;
         goto error;
      }
      av_bprintf(&buffer, "%s", home);
      ++outputFile;
   }

   time_t timeNum = time(NULL);
   struct tm *timeObj = localtime(&timeNum);
   av_bprint_strftime(&buffer, outputFile, timeObj);
   if (!av_bprint_is_complete(&buffer)) {
      ret = AVERROR(ENOMEM);
      goto error;
   }
   if ((ret = av_bprint_finalize(&buffer, &path)) < 0) {
      goto error;
   }

   av_log(NULL, AV_LOG_INFO, "Saving video to '%s'...\n", path);
   if ((ret = avformat_alloc_output_context2(&output->formatCtx, NULL, "mp4", path)) <
       0) {
      av_log(NULL, AV_LOG_ERROR, "Failed to allocate output format: %s\n",
             av_err2str(ret));
      goto error;
   }
   if ((ret = avio_open(&output->formatCtx->pb, path, AVIO_FLAG_WRITE)) < 0) {
      av_log(output->formatCtx, AV_LOG_ERROR, "Failed to open output: %s\n",
             av_err2str(ret));
      goto error;
   }
   av_freep(&path);

   return 0;
error:
   av_freep(&path);
   av_bprint_finalize(&buffer, NULL);
   rsOutputDestroy(output);
   return ret;
}

void rsOutputAddStream(RSOutput *output, const AVCodecParameters *params) {
   int ret;
   if (output->error < 0) {
      return;
   }

   AVStream *stream = avformat_new_stream(output->formatCtx, NULL);
   if (stream == NULL) {
      ret = AVERROR(ENOMEM);
      goto error;
   }
   if ((ret = avcodec_parameters_copy(stream->codecpar, params)) < 0) {
      goto error;
   }
   if (params->codec_type == AVMEDIA_TYPE_AUDIO) {
      stream->time_base = av_make_q(1, params->sample_rate);
   } else {
      stream->time_base = AV_TIME_BASE_Q;
   }

   return;
error:
   output->error = ret;
}

int rsOutputOpen(RSOutput *output) {
   int ret;
   AVDictionary *options = NULL;
   rsOptionsSet(&options, &output->error, "movflags", "+faststart");
   if (output->error < 0) {
      ret = output->error;
      goto error;
   }
   if ((ret = avformat_init_output(output->formatCtx, &options)) < 0) {
      av_log(output->formatCtx, AV_LOG_ERROR, "Failed to setup output format: %s\n",
             av_err2str(ret));
      goto error;
   }
   rsOptionsDestroy(&options);

   if ((ret = avformat_write_header(output->formatCtx, NULL)) < 0) {
      av_log(output->formatCtx, AV_LOG_ERROR, "Failed to write header: %s\n",
             av_err2str(ret));
      goto error;
   }

   return 0;
error:
   rsOptionsDestroy(&options);
   return ret;
}

int rsOutputClose(RSOutput *output) {
   int ret;
   if ((ret = av_write_trailer(output->formatCtx)) < 0) {
      av_log(output->formatCtx, AV_LOG_ERROR, "Failed to write trailer: %s\n",
             av_err2str(ret));
      return ret;
   }
   if ((ret = avio_closep(&output->formatCtx->pb)) < 0) {
      av_log(NULL, AV_LOG_ERROR, "Failed to close output: %s\n", av_err2str(ret));
   }
   return 0;
}

void rsOutputDestroy(RSOutput *output) {
   avio_closep(&output->formatCtx->pb);
   avformat_free_context(output->formatCtx);
   output->formatCtx = NULL;
}

int rsOutputWrite(RSOutput *output, AVPacket *packet) {
   int ret;
   AVStream *stream = output->formatCtx->streams[packet->stream_index];
   int64_t startTime = av_rescale_q(output->startTime, AV_TIME_BASE_Q, stream->time_base);
   packet->pts -= startTime;
   packet->dts -= startTime;
   if (packet->pts >= 0) {
      if ((ret = av_interleaved_write_frame(output->formatCtx, packet)) < 0) {
         av_log(NULL, AV_LOG_ERROR, "Failed to write packet: %s\n", av_err2str(ret));
         goto error;
      }
   }

   ret = 0;
error:
   av_packet_unref(packet);
   return ret;
}
