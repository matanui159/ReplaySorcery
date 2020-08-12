/*
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

#ifndef RS_UTIL_AUDIO_H
#define RS_UTIL_AUDIO_H
#include "../config.h"
#include "../util/circle_static.h"
#include <SDL2/SDL.h>
#include <pthread.h>
#include <time.h>

enum RSAUDIO_TYPE { RSAUDIO_AUTO, RSAUDIO_DEFAULT, RSAUDIO_SPECIFIC };

typedef struct RSAudio {
   SDL_AudioDeviceID deviceId;
   enum RSAUDIO_TYPE deviceType;
   SDL_AudioSpec ispec;
   SDL_AudioSpec ospec;
   pthread_spinlock_t sampleGetLock;
   RSCircleStatic data;
   int bitrate;
   int channels;
   size_t duration;
   size_t framerate;
   int deviceNum;
   size_t sizeBatch;
   uint32_t deviceAddTime;
   uint32_t deviceRemoveTime;
   bool systemFallback;
} RSAudio;

void rsAudioCreate(RSAudio *audio, const RSConfig *config);
void rsAudioDestroy(RSAudio *audio);
void rsAudioGetSamples(RSAudio *audio, uint8_t *newbuff, size_t rewindFrames);
void rsAudioHandleEvents(RSAudio *audio);

#endif
