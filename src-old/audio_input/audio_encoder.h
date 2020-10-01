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
#include "../config.h"
#include "../util/circle_static.h"
#include <fdk-aac/aacenc_lib.h>
#define AAC_OUTPUT_BUFFER_SIZE (1024 * 8)

typedef struct RSAudioEncoder {
   HANDLE_AACENCODER aac_enc;
   AACENC_InfoStruct aac_info;
   int samplesPerFrame;
   int frameSize;
} RSAudioEncoder;

void rsAudioEncoderCreate(RSAudioEncoder *audioenc, const RSConfig *config);
void rsAudioEncoderEncode(RSAudioEncoder *audioenc, uint8_t *samples, uint8_t *out,
                          int *numBytes, int *numSamples);

#endif
