/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "buffer.h"
#include <string.h>

void rsBufferCreate(RSBuffer *buffer, size_t capacity) {
   buffer->data = rsAllocate(capacity);
   buffer->size = 0;
   buffer->capacity = capacity;
}

void rsBufferDestroy(RSBuffer *buffer) {
   rsFree(buffer->data);
   rsClear(buffer, sizeof(RSBuffer));
}

void rsBufferClone(RSBuffer *buffer, const RSBuffer *source) {
   rsBufferClear(buffer);
   void *data = rsBufferAppend(buffer, source->size);
   memcpy(data, source->data, source->size);
}

void *rsBufferAppend(RSBuffer *buffer, size_t size) {
   size_t offset = buffer->size;
   buffer->size += size;
   if (buffer->size > buffer->capacity) {
      do {
         buffer->capacity *= 2;
      } while (buffer->size > buffer->capacity);
      buffer->data = rsReallocate(buffer->data, buffer->capacity);
   }
   return (char *)buffer->data + offset;
}

void *rsBufferGetSpace(RSBuffer *buffer, size_t *size) {
   if (buffer->size >= buffer->capacity) {
      buffer->capacity *= 2;
      buffer->data = rsReallocate(buffer->data, buffer->capacity);
   }
   *size = buffer->capacity - buffer->size;
   return (char *)buffer->data + buffer->size;
}

void rsBufferOptimize(RSBuffer *buffer) {
   if (buffer->size < buffer->capacity / 4) {
      do {
         buffer->capacity /= 2;
      } while (buffer->size < buffer->capacity / 4);
      void *data = rsAllocate(buffer->capacity);
      memcpy(data, buffer->data, buffer->size);
      rsFree(buffer->data);
      buffer->data = data;
   }
}

void rsBufferClear(RSBuffer *buffer) {
   buffer->size = 0;
}
