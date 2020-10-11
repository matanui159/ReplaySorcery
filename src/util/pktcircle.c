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

#include "pktcircle.h"

int rsPktCircleCreate(RSPktCircle *circle, size_t capacity) {
   circle->packets = av_mallocz_array(capacity, sizeof(AVPacket));
   if (circle->packets == NULL) {
      return AVERROR(ENOMEM);
   }
   for (size_t i = 0; i < capacity; ++i) {
      av_init_packet(&circle->packets[i]);
   }
   circle->capacity = capacity;
   circle->offset = 0;
   circle->size = 0;
   return 0;
}

void rsPktCircleDestroy(RSPktCircle *circle) {
   if (circle->packets != NULL) {
      for (size_t i = 0; i < circle->capacity; ++i) {
         av_packet_unref(&circle->packets[i]);
      }
      av_freep(&circle->packets);
   }
}

AVPacket *rsPktCircleNext(RSPktCircle *circle) {
   size_t index;
   if (circle->size < circle->capacity) {
      index = circle->size;
      ++circle->size;
   } else {
      index = circle->offset;
      circle->offset = (circle->offset + 1) % circle->capacity;
   }
   return &circle->packets[index];
}

int rsPktCircleClone(RSPktCircle *circle, const RSPktCircle *source) {
   int ret;
   if (circle->capacity != source->capacity) {
      return AVERROR(EINVAL);
   }
   for (size_t i = 0; i < source->size; ++i) {
      size_t index = (source->offset + i) % source->capacity;
      if ((ret = av_packet_ref(&circle->packets[i], &source->packets[index])) < 0) {
         return ret;
      }
   }

   circle->offset = 0;
   circle->size = source->size;
   return 0;
}
