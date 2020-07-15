/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

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

void rsBufferCircleExtract(RSBufferCircle *circle, RSBuffer *target) {
   for (size_t i = 0; i < circle->size; ++i) {
      size_t index = (circle->offset + i) % circle->capacity;
      RSBuffer *buffer = &circle->buffers[index];
      void *data = rsBufferGrow(target, buffer->size);
      memcpy(data, buffer->data, buffer->size);
   }
}
