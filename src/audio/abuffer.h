/*
 * Copyright (C) 2020  Joshua Minter, Patryk Seregiet
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

#ifndef RS_AUDIO_ABUFFER_H
#define RS_AUDIO_ABUFFER_H
#include "../encoder/encoder.h"
#include "../output.h"
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

typedef struct RSAudioBuffer {
   AVCodecParameters *params;
   int sampleSize;
   int8_t *data;
   int capacity;
   int index;
   int size;
   int64_t endTime;
   RSEncoder encoder;
} RSAudioBuffer;

int rsAudioBufferCreate(RSAudioBuffer *buffer, const AVCodecParameters *params);
void rsAudioBufferDestroy(RSAudioBuffer *buffer);
int rsAudioBufferAddFrame(RSAudioBuffer *buffer, AVFrame *frame);
int rsAudioBufferGetParams(RSAudioBuffer *buffer, const AVCodecParameters **params);
int rsAudioBufferWrite(RSAudioBuffer *buffer, RSOutput *output, int stream,
                       int64_t startTime);

#endif
