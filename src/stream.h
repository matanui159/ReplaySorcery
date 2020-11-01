/*
 * Copyright (C) 2020  Joshua Minter
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

#ifndef RS_STREAM_H
#define RS_STREAM_H
#include "encoder/encoder.h"
#include "rsbuild.h"
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#ifdef RS_BUILD_PTHREAD_FOUND
#include <pthread.h>
#endif

typedef struct RSPacketList {
   struct RSPacketList *next;
   AVPacket packet;
} RSPacketList;

typedef struct RSStream {
   RSEncoder *input;
   RSPacketList *pool;
   RSPacketList *tail;
   RSPacketList *head;
#ifdef RS_BUILD_PTHREAD_FOUND
   pthread_mutex_t mutex;
   int mutexCreated;
#endif
} RSStream;

int rsStreamCreate(RSStream **stream, RSEncoder *input);
void rsStreamDestroy(RSStream **stream);
int rsStreamUpdate(RSStream *stream);
AVPacket *rsStreamGetPackets(RSStream *stream, size_t *size);
void rsStreamPacketsDestroy(AVPacket **packets, size_t size);

#endif
