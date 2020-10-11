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

#ifndef RS_UTIL_PKTCIRCLE_H
#define RS_UTIL_PKTCIRCLE_H
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>

typedef struct RSPktCircle {
   AVPacket *packets;
   size_t capacity;
   size_t offset;
   size_t size;
} RSPktCircle;

#define RS_PKTCIRCLE_INIT                                                                \
   { NULL }

int rsPktCircleCreate(RSPktCircle *circle, size_t capacity);
void rsPktCircleDestroy(RSPktCircle *circle);
AVPacket *rsPktCircleNext(RSPktCircle *circle);
int rsPktCircleClone(RSPktCircle *circle, const RSPktCircle *source);

#endif
