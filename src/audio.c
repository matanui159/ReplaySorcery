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
#include "memory.h"

#define SAMPLE_COUNT 4096

static void audioCallback(void *userdata, uint8_t *stream, int len) {
	RSAudio* audio = (RSAudio*)userdata;
	int end = audio->index + len;
	if (end >= audio->size) {
		audio->index = 0;
	}
	memcpy(audio->data + audio->index, stream, (size_t)len);
	audio->index += len;
}

static const char *getDeviceName(const RSConfig *config) {
   const char* devname = config->audioDeviceName;
   if (!devname || !strcmp(config->audioDeviceName, "auto")) {
	   return 0;
   }
   return devname;
}

static void prepareEncoder(RSAudio *audio, const RSConfig* config) {
   aacEncOpen(&(audio->audioenc.aac_enc), 0, 0);
   aacEncoder_SetParam(audio->audioenc.aac_enc, AACENC_TRANSMUX, 0);
   aacEncoder_SetParam(audio->audioenc.aac_enc, AACENC_AFTERBURNER, 1);
   aacEncoder_SetParam(audio->audioenc.aac_enc, AACENC_BITRATE, AUDIO_BITRATE);
   aacEncoder_SetParam(audio->audioenc.aac_enc, AACENC_SAMPLERATE, AUDIO_RATE);
   aacEncoder_SetParam(audio->audioenc.aac_enc, AACENC_CHANNELMODE, config->audioChannels);
   aacEncEncode(audio->audioenc.aac_enc, NULL, NULL, NULL, NULL);
   aacEncInfo(audio->audioenc.aac_enc, &(audio->audioenc.aac_info));

   audio->audioenc.data = 0;
   audio->audioenc.frame = 0;
   audio->audioenc.size = audio->size;
   audio->audioenc.frame_size = (int)audio->audioenc.aac_info.frameLength * audio->channels * (int)sizeof(uint16_t);
   audio->audioenc.index = 0;
}

static void rsAudioEncoderPrepareFrame(RSAudioEncoder *audioenc) {
   int firstCopySize = (int)audioenc->frame_size;
   int endData = (int)audioenc->index + audioenc->frame_size;
   int diff = audioenc->size - endData;
   if (diff >= 0) {
      memcpy(audioenc->frame, audioenc->data + audioenc->index, (size_t)firstCopySize);
      audioenc->index += firstCopySize;
      return;
   }
   firstCopySize += diff;
   memcpy(audioenc->frame, audioenc->data + audioenc->index, firstCopySize);
   memcpy(audioenc->frame + firstCopySize, audioenc->data, -diff);
   audioenc->index = -diff;
}

int rsAudioCreate(RSAudio *audio, const RSConfig *config) {
   if (SDL_Init(SDL_INIT_AUDIO) < 0) {
   	rsError("SDL2 could not initialize! SDL Error: %s", SDL_GetError());
   }

   SDL_AudioDeviceID devId = 0;
   SDL_AudioSpec ispec;
   SDL_AudioSpec ospec;
   SDL_zero(ispec);
   SDL_zero(ospec);

   ispec.freq = (int)config->audioSamplerate;
   ispec.format = AUDIO_S16LSB;
   ispec.channels = config->audioChannels;
   ispec.samples = SAMPLE_COUNT; //must be power of 2
   ispec.callback = audioCallback;
   ispec.userdata = audio;

   const char* devname = getDeviceName(config);
   devId = SDL_OpenAudioDevice(devname, 1, &ispec, &ospec, 0);
   if (!devId) {
   	rsError("SDL2 couldn't open audio device %s", SDL_GetError());
   }
   audio->deviceId = devId;

   int size1s = ospec.channels * ospec.freq * sizeof(uint16_t);
   int sizebatch = size1s / config->framerate;
   int sizeTotal = size1s * config->duration;
   int numOfFrames = (sizeTotal + (SAMPLE_COUNT - 1)) / SAMPLE_COUNT; //round up div
   sizeTotal = numOfFrames * SAMPLE_COUNT;

   audio->size = sizeTotal;
   audio->data = malloc(sizeTotal);
   audio->index = 0;
   audio->channels = ospec.channels;
   audio->bitrate = config->audioBitrate;
   audio->sizebatch = sizebatch;
   prepareEncoder(audio, config);
   SDL_PauseAudioDevice(audio->deviceId, 0);
   return 1;
}

void rsAudioEncoderCreate(RSAudioEncoder* audioenc, const RSAudio *audio, int rewindframes) {
	SDL_PauseAudioDevice(audio->deviceId, 1);
	*audioenc = audio->audioenc;
	audioenc->data = malloc(audioenc->size);
	audioenc->frame = malloc(audioenc->frame_size); 
	memcpy(audioenc->data, audio->data, audio->size);
	size_t rewind_bytes = rewindframes * audio->sizebatch;
	if (rewind_bytes <= audio->index) {
		audioenc->index = audio->index - rewind_bytes;
		return;
	}
	audioenc->index = audio->size - (rewind_bytes - audio->index);
	SDL_PauseAudioDevice(audio->deviceId, 0);
}

void rsAudioDestroy(RSAudio *audio) {
   if (audio->deviceId) {
	   SDL_CloseAudioDevice(audio->deviceId);
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

void rsAudioEncodeFrame(RSAudioEncoder *audioenc, uint8_t *out, int *num_of_bytes, int *num_of_samples) {
   AACENC_BufDesc ibuf = {0};
   AACENC_BufDesc obuf = {0};
   AACENC_InArgs iargs = {0};
   AACENC_OutArgs oargs = {0};

   rsAudioEncoderPrepareFrame(audioenc);
   void *iptr = audioenc->frame;
   void *optr = out;

   int ibufsizes = audioenc->frame_size;
   int iidentifiers = IN_AUDIO_DATA;
   int ielsizes = 2;
   int obufsizes = 2048;
   int oidentifiers = OUT_BITSTREAM_DATA;
   int oelsizes = 1;

   iargs.numInSamples = audioenc->aac_info.frameLength * AUDIO_CHANNELS;
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
   *num_of_bytes = oargs.numOutBytes;
   *num_of_samples = iargs.numInSamples;
}


