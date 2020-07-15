/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

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
      buffer->data = data;
   }
}
