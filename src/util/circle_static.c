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
#include "circle_static.h"
#include "memory.h"

void rsCircleStaticCreate(RSCircleStatic *circle, int size) {
   circle->data = rsMemoryCreate((size_t)size);
   circle->size = size;
   circle->index = 0;
}

void rsCircleStaticDestroy(RSCircleStatic *circle) {
   if (circle->data) {
      rsMemoryDestroy(circle->data);
   }
}

void rsCircleStaticAdd(RSCircleStatic *circle, uint8_t *src, int size) {
   int end = circle->index + size;
   int diff = circle->size - end;
   if (diff >= 0) {
      memcpy(circle->data + circle->index, src, (size_t)size);
      circle->index += size;
      return;
   }
   int part1 = size + diff;
   int part2 = size - part1;
   memcpy(circle->data + circle->index, src, (size_t)part1);
   memcpy(circle->data, src + part1, (size_t)part2);
   circle->index = part2;
}

void rsCircleStaticGet(RSCircleStatic *circle, uint8_t *dst, int size) {
   int end = circle->index + size;
   int diff = circle->size - end;
   if (diff >= 0) {
      memcpy(dst, circle->data + circle->index, (size_t)size);
      circle->index += size;
      return;
   }
   int part1 = size + diff;
   int part2 = size - part1;
   memcpy(dst, circle->data + circle->index, (size_t)part1);
   memcpy(dst + part1, circle->data, (size_t)part2);
   circle->index = part2;
}

void rsCircleStaticMoveBackIndex(RSCircleStatic *circle, int bytes) {
   if (bytes <= circle->index) {
      circle->index -= bytes;
      return;
   }
   circle->index = circle->size - (bytes - circle->index);
}
