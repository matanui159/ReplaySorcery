/*
 * Copyright (C) 2020  Joshua Minter & Patryk Seregiet
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

#include "audio.h"
#include "util/log.h"
#include "util/memory.h"

static void rsAudioReadSamples(RSAudio *audio);
static void rsAudioPrepareEncoder(RSAudio *audio, const RSConfig* config);
static void rsAudioEncoderPrepareFrame(RSAudioEncoder *audioenc);

extern sig_atomic_t mainRunning;
extern sig_atomic_t wantToSave;

int rsAudioCreate(RSAudio *audio, RSConfig *config) {
   uint8_t channels = config->audioChannels;
   if (channels != 1 && channels != 2) {
      rsLog("Only 1 or 2 channels are supported. Setting to 2");
   }

   int error;
   const pa_sample_spec ss = {
       .format = PA_SAMPLE_S16LE, 
       .rate = config->audioSamplerate, 
       .channels = channels
   };
   audio->paApi = pa_simple_new(NULL, "Replay Sorcery", PA_STREAM_RECORD, NULL,
                                 "record", &ss, NULL, NULL, &error);

   if (!audio->paApi) {
      rsError("PulseAudio: pa_simple_new() failed: %s\n", pa_strerror(error));
      return 0;
   }
   int size1s = pa_bytes_per_second(&ss);
   int sizePerFrame = size1s / config->framerate;
   int sizeTotal = size1s * config->duration;
   audio->sizeBatch = sizePerFrame;
   audio->size = sizeTotal;
   audio->data = rsMemoryCreate(sizeTotal);
   audio->index = 0;
   rsAudioPrepareEncoder(audio, config);
   return 1;
}

void rsAudioEncoderCreate(RSAudioEncoder* audioenc, const RSAudio *audio, int rewindFrames) {
	*audioenc = audio->audioenc;
	audioenc->data = rsMemoryCreate(audioenc->size);
	audioenc->frame = rsMemoryCreate(audioenc->frameSize); 
	memcpy(audioenc->data, audio->data, audio->size);
	size_t rewindBytes = rewindFrames * audio->sizeBatch;
	if (rewindBytes <= audio->index) {
		audioenc->index = audio->index - rewindBytes;
		return;
	}
	audioenc->index = audio->size - (rewindBytes - audio->index);
}

void rsAudioDestroy(RSAudio *audio) {
   if (audio->paApi) {
      pa_simple_free(audio->paApi);
   }
   if (audio->data) {
      free(audio->data);
   }
}

void rsAudioEncoderDestroy(RSAudioEncoder* audioenc) {
	if (audioenc->data) {
		rsMemoryDestroy(audioenc->data);
	}
	if (audioenc->frame) {
		rsMemoryDestroy(audioenc->frame);
	}
}

void rsAudioEncodeFrame(RSAudioEncoder *audioenc, uint8_t *out, int *numBytes, int *numSamples) {
   AACENC_BufDesc ibuf = {0};
   AACENC_BufDesc obuf = {0};
   AACENC_InArgs iargs = {0};
   AACENC_OutArgs oargs = {0};

   rsAudioEncoderPrepareFrame(audioenc);
   void *iptr = audioenc->frame;
   void *optr = out;

   int ibufsizes = audioenc->frameSize;
   int iidentifiers = IN_AUDIO_DATA;
   int ielsizes = 2;
   int obufsizes = 2048;
   int oidentifiers = OUT_BITSTREAM_DATA;
   int oelsizes = 1;

   iargs.numInSamples = audioenc->samplesPerFrame;
   ibuf.numBufs = 1;
   ibuf.bufs = &iptr;
   ibuf.bufferIdentifiers = &iidentifiers;
   ibuf.bufSizes = &ibufsizes;
   ibuf.bufElSizes = &ielsizes;

   obuf.numBufs = 1;
   obuf.bufs = &optr;
   obuf.bufferIdentifiers = &oidentifiers;
   obuf.bufSizes = &obufsizes;
   obuf.bufElSizes = &oelsizes;

   if (AACENC_OK !=
       aacEncEncode(audioenc->aac_enc, &ibuf, &obuf, &iargs, &oargs)) {
      rsLog("AAC: aac encode failed");
   }
   *numBytes = oargs.numOutBytes;
   *numSamples = iargs.numInSamples;
}

void *rsAudioThread(void *data) {
   RSAudio *audio = (RSAudio *)data;
   while (!wantToSave && mainRunning) {
      rsAudioReadSamples(audio);
   }
   return 0;
}

static void rsAudioReadSamples(RSAudio *audio) {
   int error;
   int ret = pa_simple_read(audio->paApi, audio->data + audio->index, audio->sizeBatch,
                        &error);
   if (ret < 0) {
      rsError("PulseAudio: pa_simple_read() failed: %s", pa_strerror(error));
   }
   audio->index += audio->sizeBatch;
   if (audio->index >= audio->size) {
      audio->index -= audio->size;
   }
}

static void rsAudioPrepareEncoder(RSAudio *audio, const RSConfig* config) {
   RSAudioEncoder* audioenc = &audio->audioenc;
   if (AACENC_OK != aacEncOpen(&(audioenc->aac_enc), 0, 0)) {
      rsError("aacEncOpen failed");
   }
   aacEncoder_SetParam(audioenc->aac_enc, AACENC_TRANSMUX, 0);
   aacEncoder_SetParam(audioenc->aac_enc, AACENC_AFTERBURNER, 1);
   aacEncoder_SetParam(audioenc->aac_enc, AACENC_BITRATE, config->audioBitrate);
   aacEncoder_SetParam(audioenc->aac_enc, AACENC_SAMPLERATE, config->audioSamplerate);
   aacEncoder_SetParam(audioenc->aac_enc, AACENC_CHANNELMODE, config->audioChannels);
   if (AACENC_OK != aacEncEncode(audioenc->aac_enc, NULL, NULL, NULL, NULL)) {
      rsError("aacEncEncode failed. Probably bad bitrate. Tried %d", config->audioBitrate);
   }
   aacEncInfo(audioenc->aac_enc, &(audio->audioenc.aac_info));

   audioenc->data = 0;
   audioenc->frame = 0;
   audioenc->index = 0;
   audioenc->size = audio->size;
   audioenc->samplesPerFrame = audioenc->aac_info.frameLength * config->audioChannels;
   audioenc->frameSize = audioenc->samplesPerFrame * sizeof(uint16_t);
}

static void rsAudioEncoderPrepareFrame(RSAudioEncoder *audioenc) {
   int firstCopySize = audioenc->frameSize;
   int endOfData = audioenc->index + audioenc->frameSize;
   int diff = audioenc->size - endOfData;
   if (diff >= 0) {
      memcpy(audioenc->frame, audioenc->data + audioenc->index, firstCopySize);
      audioenc->index += firstCopySize;
      return;
   }
   firstCopySize += diff;
   memcpy(audioenc->frame, audioenc->data + audioenc->index, firstCopySize);
   memcpy(audioenc->frame + firstCopySize, audioenc->data, -diff);
   audioenc->index = -diff;
}

