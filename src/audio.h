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

#ifndef RS_UTIL_AUDIO_H
#define RS_UTIL_AUDIO_H

#include "config.h"
#include <fdk-aac/aacenc_lib.h>
#include <pulse/error.h>
#include <pulse/simple.h>

typedef struct RSAudioEncoder {
   HANDLE_AACENCODER aac_enc;
   AACENC_InfoStruct aac_info;
   uint8_t *data;
   uint8_t *frame;
   int size;
   int index;
   int frameSize;
   int samplesPerFrame;
} RSAudioEncoder;


typedef struct RSAudio {
   pa_simple *paApi;
   uint8_t *data;
   int size;
   int index;
   int sizeBatch;
   RSAudioEncoder audioenc;
} RSAudio;

void rsAudioEncodeFrame(RSAudioEncoder *audioenc, uint8_t *out, int *numBytes, int *numSamples);
void rsAudioEncoderCreate(RSAudioEncoder* audioenc, const RSAudio *audio, int rewindFrames);
void rsAudioEncoderDestroy(RSAudioEncoder* audioenc);

int rsAudioCreate(RSAudio *audio, RSConfig *config);
void rsAudioDestroy(RSAudio *audio);
void *rsAudioThread(void *data);


#endif

