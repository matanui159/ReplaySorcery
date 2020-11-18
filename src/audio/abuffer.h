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
#include "../output.h"
#include <libavformat/avformat.h>

typedef struct RSAudioBuffer {
   int8_t *data;
   size_t capacity;
   size_t index;
   size_t size;
   int64_t endTime;
} RSAudioBuffer;

int rsAudioBufferCreate(RSAudioBuffer *buffer);
void rsAudioBufferDestroy(RSAudioBuffer *buffer);
int rsAudioBufferAddFrame(RSAudioBuffer *buffer, AVFrame *frame);
int rsAudioBufferWrite(RSOutput *output, int64_t startTime);

#endif
