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
#include "../util/log.h"
#include "../util/memory.h"
#define SAMPLES_PER_CALLBACK 4096 // has to be power of 2

static void audioCallback(void *userdata, uint8_t *stream, int len) {
   RSAudio *audio = (RSAudio *)userdata;
   rsCircleStaticAdd(&audio->data, stream, len);
}

static void getDeviceName(RSAudio *audio, const RSConfig *config) {
   const char *monitor = "Monitor of ";
   const char *devname = config->audioDeviceName;
   if (!strcmp(config->audioDeviceName, "unknown")) {
      audio->deviceName = NULL;
      return;
   }
   if (!strcmp(devname, "auto")) {
      const char *sdlname = SDL_GetAudioDeviceName(0, false);
      if (!sdlname) {
         audio->deviceName = NULL;
	 return;
      }
      audio->deviceName = rsMemoryCreate(strlen(sdlname) + strlen(monitor) + 1);
      strcpy(audio->deviceName, monitor);
      strcat(audio->deviceName, sdlname);
      return;
   }
   if (!strstr(devname, monitor)) {
      audio->deviceName = rsMemoryCreate(strlen(devname) + strlen(monitor) + 1);
      strcpy(audio->deviceName, monitor);
      strcat(audio->deviceName, devname);
      return;
   }
   audio->deviceName = rsMemoryCreate(strlen(devname) + 1);
   strcpy(audio->deviceName, devname);
}

static void printDevices(void) {
   rsLog("Available devices:");
   int max = SDL_GetNumAudioDevices(true);
   for (int i = 0; i < max; ++i) {
      rsLog(SDL_GetAudioDeviceName(i, true));
   }
}

void rsAudioCreate(RSAudio *audio, const RSConfig *config) {
   if (SDL_Init(SDL_INIT_AUDIO) < 0) {
      rsError("SDL2 could not initialize! SDL Error: %s", SDL_GetError());
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
   ispec.samples = SAMPLES_PER_CALLBACK; // must be power of 2
   ispec.callback = audioCallback;
   ispec.userdata = audio;

   getDeviceName(audio, config);
   devId = SDL_OpenAudioDevice(audio->deviceName, true, &ispec, &ospec, 0);
   if (!devId) {
      rsError("SDL2 couldn't open audio device %s", SDL_GetError());
   }
   rsLog("Opened audio device %s", audio->deviceName);

   audio->deviceId = devId;
   int size1s = ospec.channels * ospec.freq * (int)sizeof(uint16_t);
   int sizeTotal = size1s * config->duration;
   int numOfCallbacks =
       (sizeTotal + (SAMPLES_PER_CALLBACK - 1)) / SAMPLES_PER_CALLBACK; // round up div
   sizeTotal = numOfCallbacks * SAMPLES_PER_CALLBACK;
   audio->sizeBatch = size1s / config->framerate;

   rsCircleStaticCreate(&audio->data, sizeTotal);
   SDL_PauseAudioDevice(audio->deviceId, 0);
}

void rsAudioGetSamples(RSAudio *audio, uint8_t *newbuff, int rewindFrames) {
   int oldIndex = audio->data.index;
   SDL_LockAudioDevice(audio->deviceId);
   rsCircleStaticMoveBackIndex(&audio->data, rewindFrames * audio->sizeBatch);
   rsCircleStaticGet(&audio->data, newbuff, audio->data.size);
   audio->data.index = oldIndex;
   SDL_UnlockAudioDevice(audio->deviceId);
}

void rsAudioDestroy(RSAudio *audio) {
   if (audio->deviceId) {
      SDL_CloseAudioDevice(audio->deviceId);
   }
   if (audio->deviceName) {
      rsMemoryDestroy(audio->deviceName);
   }
   rsCircleStaticDestroy(&audio->data);
}
