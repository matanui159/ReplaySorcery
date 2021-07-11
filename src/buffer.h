/*
 * Copyright (C) 2020-2021  Joshua Minter
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
#include "output.h"
#include "rsbuild.h"
#include <libavcodec/avcodec.h>

typedef struct RSPacketList {
   struct RSPacketList *next;
   AVPacket *packet;
} RSPacketList;

typedef struct RSBuffer {
   RSPacketList *pool;
   RSPacketList *tail;
   RSPacketList *head;
} RSBuffer;

int rsBufferCreate(RSBuffer *buffer);
void rsBufferDestroy(RSBuffer *buffer);
int rsBufferAddPacket(RSBuffer *buffer, AVPacket *packet);
int64_t rsBufferGetStartTime(RSBuffer *buffer);
int rsBufferWrite(RSBuffer *buffer, RSOutput *output, int stream);

#endif
