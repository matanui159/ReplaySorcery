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

#ifndef RS_AUDIO_ENCODER_H
#define RS_AUDIO_ENCODER_H
#include <fdk-aac/aacenc_lib.h>
#include "../util/circle_static.h"
#include "../config.h"

typedef struct RSAudioEncoder {
   HANDLE_AACENCODER aac_enc;
   AACENC_InfoStruct aac_info;
   RSCircleStatic data;
   uint8_t *frame;
   int size;
   int frameSize;
   int index;
   int samplesPerFrame;
} RSAudioEncoder;

void rsAudioEncodeFrame(RSAudioEncoder *audioenc, uint8_t *out, int *numBytes, int *numSamples);
void rsAudioEncoderCreate(RSAudioEncoder* audioenc, RSAudio *audio, int rewindFrames);
void rsAudioEncoderDestroy(RSAudioEncoder* audioenc);

#endif

