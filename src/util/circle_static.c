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

void rsCircleStaticCreate(RSCircleStatic *circle, size_t size) {
   circle->data = rsMemoryCreate(size);
   circle->size = size;
   circle->index = 0;
}

void rsCircleStaticDestroy(RSCircleStatic *circle) {
   if (circle->data != NULL) {
      rsMemoryDestroy(circle->data);
   }
}

void rsCircleStaticAdd(RSCircleStatic *circle, uint8_t *src, size_t size) {
   size_t end = circle->index + size;
   if (circle->size >= end) {
      memcpy(circle->data + circle->index, src, size);
      circle->index = end;
      return;
   }
   size_t part1 = circle->size - circle->index;
   size_t part2 = size - part1;
   memcpy(circle->data + circle->index, src, part1);
   memcpy(circle->data, src + part1, part2);
   circle->index = part2;
}

void rsCircleStaticGet(RSCircleStatic *circle, uint8_t *dst, size_t size) {
   size_t end = circle->index + size;
   if (circle->size >= end) {
      memcpy(dst, circle->data + circle->index, size);
      circle->index += size;
      return;
   }
   size_t part1 = size - (end - circle->size);
   size_t part2 = size - part1;
   memcpy(dst, circle->data + circle->index, part1);
   memcpy(dst + part1, circle->data, part2);
   circle->index = part2;
}

void rsCircleStaticMoveBackIndex(RSCircleStatic *circle, size_t bytes) {
   if (bytes <= circle->index) {
      circle->index -= bytes;
      return;
   }
   circle->index = circle->size - (bytes - circle->index);
}
