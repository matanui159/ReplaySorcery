/*
 * Copyright (C) 2020 Patryk Seregiet
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
#define SAMPLES_PER_CALLBACK 4096 //has to be power of 2

static void audioCallback(void *userdata, uint8_t *stream, int len) {
   RSAudio* audio = (RSAudio*)userdata;
   pthread_spin_lock(&audio->sampleGetLock);
   rsCircleStaticAdd(&audio->data, stream, len);
   pthread_spin_unlock(&audio->sampleGetLock);
}

static const char *getDeviceName(const RSConfig *config) {
   const char* devname = config->audioDeviceName;
   if (!devname || !strcmp(config->audioDeviceName, "auto")) {
      return NULL;
   }
   return devname;
}

static void printDevices(void) {
   rsLog("Available devices:");
   int max = SDL_GetNumAudioDevices(1);
   for (int i = 0; i < max; ++i) {
      rsLog(SDL_GetAudioDeviceName(i, 1));
   }
}

static void prepareEncoder(RSAudio *audio, const RSConfig* config) {
   RSAudioEncoder* audioenc = &audio->audioenc;
   if (AACENC_OK != aacEncOpen(&(audioenc->aac_enc), 0, 0)) {
      rsError("aacEncOpen failed");
   }
   aacEncoder_SetParam(audioenc->aac_enc, AACENC_TRANSMUX, 0);
   aacEncoder_SetParam(audioenc->aac_enc, AACENC_AFTERBURNER, 1);
   aacEncoder_SetParam(audioenc->aac_enc, AACENC_BITRATE, (UINT)config->audioBitrate);
   aacEncoder_SetParam(audioenc->aac_enc, AACENC_SAMPLERATE, (UINT)config->audioSamplerate);
   aacEncoder_SetParam(audioenc->aac_enc, AACENC_CHANNELMODE, (UINT)config->audioChannels);
   if (AACENC_OK != aacEncEncode(audioenc->aac_enc, NULL, NULL, NULL, NULL)) {
      rsError("aacEncEncode failed. Probably bad bitrate. Tried %d", config->audioBitrate);
   }
   aacEncInfo(audioenc->aac_enc, &(audio->audioenc.aac_info));

   audioenc->data.data = 0;
   audioenc->frame = 0;
   audioenc->index = 0;
   audioenc->size = audio->size;
   audioenc->samplesPerFrame = (int)audioenc->aac_info.frameLength * config->audioChannels;
   audioenc->frameSize = audioenc->samplesPerFrame * (int)sizeof(uint16_t);
}

void rsAudioCreate(RSAudio *audio, const RSConfig *config) {
   if (SDL_Init(SDL_INIT_AUDIO) < 0) {
      rsError("SDL2 could not initialize! SDL Error: %s", SDL_GetError());
   }
   if (pthread_spin_init(&audio->sampleGetLock, PTHREAD_PROCESS_PRIVATE)) {
      rsError("pthread_spin_init failed");
   }
   printDevices();
   SDL_AudioDeviceID devId = 0;
   SDL_AudioSpec ispec;
   SDL_AudioSpec ospec;
   SDL_zero(ispec);
   SDL_zero(ospec);

   ispec.freq = (int)config->audioSamplerate;
   ispec.format = AUDIO_S16LSB;
   ispec.channels = (Uint8)config->audioChannels;
   ispec.samples = SAMPLES_PER_CALLBACK; //must be power of 2
   ispec.callback = audioCallback;
   ispec.userdata = audio;

   const char* devname = getDeviceName(config);
   devId = SDL_OpenAudioDevice(devname, 1, &ispec, &ospec, 0);
   if (!devId) {
      rsError("SDL2 couldn't open audio device %s", SDL_GetError());
   }

   audio->deviceId = devId;
   int size1s = ospec.channels * ospec.freq * (int)sizeof(uint16_t);
   int sizebatch = size1s / config->framerate;
   int sizeTotal = size1s * config->duration;
   int numOfCallbacks = (sizeTotal + (SAMPLES_PER_CALLBACK - 1)) / SAMPLES_PER_CALLBACK; //round up div
   sizeTotal = numOfCallbacks * SAMPLES_PER_CALLBACK;

   rsCircleStaticCreate(&audio->data, sizeTotal);
   audio->size = sizeTotal;
   audio->index = 0;
   audio->channels = ospec.channels;
   audio->bitrate = config->audioBitrate;
   audio->sizeBatch = sizebatch;
   prepareEncoder(audio, config);
   SDL_PauseAudioDevice(audio->deviceId, 0);
}

void rsAudioEncoderCreate(RSAudioEncoder* audioenc, RSAudio *audio, int rewindFrames) {
   pthread_spin_lock(&audio->sampleGetLock);
   *audioenc = audio->audioenc;
   rsCircleStaticDuplicate(&audioenc->data, &audio->data);
   int rewindBytes = rewindFrames * audio->sizeBatch;
   rsCircleStaticMoveBackIndex(&audioenc->data, rewindBytes);
   audioenc->frame = rsMemoryCreate((size_t)audioenc->frameSize); 
   pthread_spin_unlock(&audio->sampleGetLock);
}

void rsAudioDestroy(RSAudio *audio) {
   if (audio->deviceId) {
      SDL_CloseAudioDevice(audio->deviceId);
   }
   rsCircleStaticDestroy(&audio->data);
   pthread_spin_destroy(&audio->sampleGetLock);
}

void rsAudioEncoderDestroy(RSAudioEncoder* audioenc) {
   if (audioenc->frame) {
      rsMemoryDestroy(audioenc->frame);
   }
   rsCircleStaticDestroy(&audioenc->data);
}

void rsAudioEncodeFrame(RSAudioEncoder *audioenc, uint8_t *out, int *num_of_bytes, int *num_of_samples) {
   AACENC_BufDesc ibuf = {0};
   AACENC_BufDesc obuf = {0};
   AACENC_InArgs iargs = {0};
   AACENC_OutArgs oargs = {0};

   //prepareFrame(audioenc);
   rsCircleStaticGet(&audioenc->data, audioenc->frame, audioenc->frameSize);
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
   *num_of_bytes = oargs.numOutBytes;
   *num_of_samples = iargs.numInSamples;
}


