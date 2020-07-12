/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "buffer.h"

void rsCreateBuffer(RSBuffer *buffer, size_t capacity) {
   buffer->data = rsAllocate(capacity);
   buffer->size = 0;
   buffer->capacity = capacity;
}

void rsDestroyBuffer(RSBuffer *buffer) {
   rsFree(buffer->data);
   rsClear(buffer, sizeof(RSBuffer));
}

void *rsAppendBuffer(RSBuffer *buffer, size_t size) {
   size_t offset = buffer->size;
   buffer->size += size;
   if (buffer->size > buffer->capacity) {
      do {
         buffer->capacity *= 2;
      } while (buffer->size > buffer->capacity);
      buffer->data = rsReallocate(buffer->data, buffer->capacity);
   }
   return (char*)buffer->data + offset;
}

void *rsGetBufferSpace(RSBuffer *buffer, size_t *size) {
   if (buffer->size >= buffer->capacity) {
      buffer->capacity *= 2;
      buffer->data = rsReallocate(buffer->data, buffer->capacity);
   }
   *size = buffer->capacity - buffer->size;
   return (char*)buffer->data + buffer->size;
}

void rsClearBuffer(RSBuffer *buffer) {
   buffer->size = 0;
}
