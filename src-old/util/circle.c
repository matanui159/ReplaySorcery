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

#include "circle.h"
#include "memory.h"

void rsBufferCircleCreate(RSBufferCircle *circle, size_t capacity) {
   circle->buffers = rsMemoryCreate(capacity * sizeof(RSBuffer));
   for (size_t i = 0; i < capacity; ++i) {
      rsBufferCreate(&circle->buffers[i]);
   }
   circle->size = 0;
   circle->capacity = capacity;
   circle->offset = 0;
}

void rsBufferCircleDestroy(RSBufferCircle *circle) {
   for (size_t i = 0; i < circle->capacity; ++i) {
      rsBufferDestroy(&circle->buffers[i]);
   }
   rsMemoryDestroy(circle->buffers);
   rsMemoryClear(circle, sizeof(RSBufferCircle));
}

RSBuffer *rsBufferCircleNext(RSBufferCircle *circle) {
   size_t index;
   if (circle->size < circle->capacity) {
      index = circle->size;
      ++circle->size;
   } else {
      index = circle->offset;
      circle->offset = (circle->offset + 1) % circle->capacity;
   }
   RSBuffer *buffer = &circle->buffers[index];
   buffer->size = 0;
   return buffer;
}

void rsBufferCircleExtract(const RSBufferCircle *circle, RSBuffer *target) {
   for (size_t i = 0; i < circle->size; ++i) {
      size_t index = (circle->offset + i) % circle->capacity;
      RSBuffer *buffer = &circle->buffers[index];
      void *data = rsBufferGrow(target, buffer->size);
      memcpy(data, buffer->data, buffer->size);
   }
}
