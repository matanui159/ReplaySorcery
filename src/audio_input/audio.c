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
#include "../util/string.h"
#define SAMPLES_PER_CALLBACK 4096 // has to be power of 2
#define AUDIO_DEVICE_ADD_WAIT_TIME_MS 500

static void audioCallback(void *userdata, uint8_t *stream, int len) {
   RSAudio *audio = (RSAudio *)userdata;
   pthread_spin_lock(&audio->sampleGetLock);
   rsCircleStaticAdd(&audio->data, stream, (size_t)len);
   pthread_spin_unlock(&audio->sampleGetLock);
}

static char *getDeviceName(const char *devname) {
   const char *monitor = "Monitor of ";
   if (devname == NULL || strcmp(devname, "system") == 0) {
      return NULL;
   }
   if (strcmp(devname, "auto") == 0) {
      const char *sdlname = SDL_GetAudioDeviceName(-1, false);
      if (sdlname == NULL) {
         return NULL;
      }
      char *newname = rsMemoryCreate(strlen(sdlname) + strlen(monitor) + 1);
      strcpy(newname, monitor);
      strcat(newname, sdlname);
      return newname;
   }
   return rsStringClone(devname);
}

static void probeDevices(RSAudio *audio) {
   int max = SDL_GetNumAudioDevices(true);
   rsLog("Available devices:");
   for (int i = 0; i < max; ++i) {
      rsLog(" - %s", SDL_GetAudioDeviceName(i, true));
   }
   audio->deviceNum = max;
}

static bool openDevice(RSAudio *audio, const char *devname) {
   SDL_zero(audio->ospec);
   audio->deviceId = SDL_OpenAudioDevice(devname, true, &audio->ispec, &audio->ospec, 0);
   if (audio->deviceId == 0) {
      rsLog("SDL2 couldn't open audio device %s : %s", SDL_GetError(), devname);
      return false;
   }
   rsLog("Opened audio device %s", devname);
   return true;
}

static void calcAudioParams(RSAudio *audio) {
   if (audio->data.data != NULL) {
      return;
   }
   size_t size1s = (size_t)(audio->ospec.channels * audio->ospec.freq) * sizeof(uint16_t);
   size_t sizeTotal = size1s * audio->duration;
   size_t numOfCallbacks =
       (sizeTotal + (SAMPLES_PER_CALLBACK - 1)) / SAMPLES_PER_CALLBACK; // round up div
   sizeTotal = numOfCallbacks * SAMPLES_PER_CALLBACK;
   audio->sizeBatch = size1s / audio->framerate;
   rsCircleStaticCreate(&audio->data, (size_t)sizeTotal);
   audio->deviceAddTime = 0;
   audio->deviceRemoveTime = 0;
}
static void deviceConnect(RSAudio *audio, const char *devname) {
   probeDevices(audio);
   char *newname = getDeviceName(devname);
   bool ok = openDevice(audio, newname);
   rsMemoryDestroy(newname);
   if (!ok) {
      if (newname != NULL && audio->systemFallback) {
         ok = openDevice(audio, NULL);
      }
      if (!ok) {
         rsLog("Couldn't open any audio device. You will have no sound");
         return;
      }
   }
   calcAudioParams(audio);
   SDL_PauseAudioDevice(audio->deviceId, 0);
}
static void deviceReconnect(RSAudio *audio, const char *devname) {
   SDL_CloseAudioDevice(audio->deviceId);
   /*
    * set the audio buffor index to 0. Effectively erasing all
    * the collected samples so far. This ensures that we'll be
    * back in sync after the audio setup is complete.
    * Once the frame pace and audio timestamps are implemented
    * it should be calculated how long the audio setup took
    * and move the index accordingly
    */
   audio->data.index = 0;
   deviceConnect(audio, devname);
}

void rsAudioCreate(RSAudio *audio, const RSConfig *config) {
   rsMemoryClear(audio, sizeof(*audio));
   if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_EVENTS) < 0) {
      rsError("SDL2 could not initialize! SDL Error: %s", SDL_GetError());
   }
   SDL_EventState(SDL_AUDIODEVICEADDED, SDL_ENABLE);
   SDL_EventState(SDL_AUDIODEVICEREMOVED, SDL_ENABLE);
   if (pthread_spin_init(&audio->sampleGetLock, PTHREAD_PROCESS_PRIVATE)) {
      rsError("pthread_spin_init failed");
   }
   audio->systemFallback = (config->audioSystemFallback != 0);
   audio->duration = (size_t)config->duration;
   audio->framerate = (size_t)config->framerate;

   audio->ispec.freq = (int)config->audioSamplerate;
   audio->ispec.format = AUDIO_S16LSB;
   audio->ispec.channels = (Uint8)config->audioChannels;
   audio->ispec.samples = SAMPLES_PER_CALLBACK; // must be power of 2
   audio->ispec.callback = audioCallback;
   audio->ispec.userdata = audio;
   deviceConnect(audio, config->audioDeviceName);
}

void rsAudioGetSamples(RSAudio *audio, uint8_t *newbuff, size_t rewindFrames) {
   size_t oldIndex = audio->data.index;
   pthread_spin_lock(&audio->sampleGetLock);
   rsCircleStaticMoveBackIndex(&audio->data, rewindFrames * audio->sizeBatch);
   rsCircleStaticGet(&audio->data, newbuff, audio->data.size);
   audio->data.index = oldIndex;
   pthread_spin_unlock(&audio->sampleGetLock);
}

void rsAudioHandleEvents(RSAudio *audio) {
   if (audio->deviceAddTime != 0 && (SDL_GetTicks() > audio->deviceAddTime)) {
      rsLog("New audio devices detected. Attempting reconnect");
      deviceReconnect(audio, "auto");
      audio->deviceAddTime = 0;
   }
   if (audio->deviceRemoveTime != 0 && (SDL_GetTicks() > audio->deviceRemoveTime)) {
      rsLog("Audio device disconnected. Trying to connect to another device");
      deviceReconnect(audio, "auto");
      audio->deviceRemoveTime = 0;
   }

   SDL_Event e;
   while (SDL_PollEvent(&e)) {
      if (e.type == SDL_AUDIODEVICEREMOVED && e.adevice.iscapture) {
         /*
          * do it after few miliseconds, give time to the pulse audio to elect
          * the new default device
          */
         audio->deviceRemoveTime = SDL_GetTicks() + AUDIO_DEVICE_ADD_WAIT_TIME_MS;
      } else if (e.type == SDL_AUDIODEVICEADDED && e.adevice.iscapture) {
         if ((int)e.adevice.which < audio->deviceNum) {
            // pulse audio sometimes reports already existing device as added
            continue;
         }
         /*
          * connecting one physical device can produce many logical devices
          * f.e a headset with microphone will produce 4 devices. speakers,
          * microphone and a monitor of each. That's why we're gonna wait few
          * milliseconds for all the devices to pop up, then try connecting
          */
         audio->deviceAddTime = SDL_GetTicks() + AUDIO_DEVICE_ADD_WAIT_TIME_MS;
      }
   }
}

void rsAudioDestroy(RSAudio *audio) {
   if (audio->deviceId) {
      SDL_CloseAudioDevice(audio->deviceId);
   }
   rsCircleStaticDestroy(&audio->data);
   pthread_spin_destroy(&audio->sampleGetLock);
}
