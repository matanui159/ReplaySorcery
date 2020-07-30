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

#include "buffer.h"
#include "memory.h"

#define BUFFER_CREATE_CAP 1024
#define BUFFER_GROW_MUL 2
#define BUFFER_GROW_MIN 1024
#define BUFFER_SHRINK_DIV 4

static bool bufferShouldShrink(RSBuffer *buffer) {
   return buffer->capacity > BUFFER_CREATE_CAP &&
          buffer->size <= buffer->capacity / BUFFER_SHRINK_DIV;
}

void rsBufferCreate(RSBuffer *buffer) {
   buffer->data = rsMemoryCreate(BUFFER_CREATE_CAP);
   buffer->size = 0;
   buffer->capacity = BUFFER_CREATE_CAP;
}

void rsBufferDestroy(RSBuffer *buffer) {
   rsMemoryDestroy(buffer->data);
   rsMemoryClear(buffer, sizeof(RSBuffer));
}

void *rsBufferGrow(RSBuffer *buffer, size_t size) {
   size_t offset = buffer->size;
   buffer->size += size;
   if (buffer->size > buffer->capacity) {
      do {
         buffer->capacity *= BUFFER_GROW_MUL;
      } while (buffer->size > buffer->capacity);
      buffer->data = rsMemoryResize(buffer->data, buffer->capacity);
   }
   return buffer->data + offset;
}

void *rsBufferAutoGrow(RSBuffer *buffer, size_t *size) {
   size_t offset = buffer->size;
   void *data = rsBufferGrow(buffer, BUFFER_GROW_MIN);
   buffer->size = buffer->capacity;
   *size = buffer->size - offset;
   return data;
}

void rsBufferShrink(RSBuffer *buffer, size_t size) {
   buffer->size -= size;
   if (bufferShouldShrink(buffer)) {
      do {
         buffer->capacity /= BUFFER_GROW_MUL;
      } while (bufferShouldShrink(buffer));
      void *data = rsMemoryCreate(buffer->capacity);
      memcpy(data, buffer->data, buffer->size);
      rsMemoryDestroy(buffer->data);
      buffer->data = data;
   }
}
