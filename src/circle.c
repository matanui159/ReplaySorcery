/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "circle.h"

static RSBuffer *bufferCircleGet(RSBufferCircle *circle, size_t index) {
   return &circle->buffers[(circle->offset + index) % circle->capacity];
}

void rsBufferCircleCreate(RSBufferCircle *circle, const RSConfig *config) {
   circle->capacity = (size_t)(config->framerate * config->duration);
   circle->buffers = rsAllocate(circle->capacity * sizeof(RSBuffer));
   for (size_t i = 0; i < circle->capacity; ++i) {
      rsBufferCreate(&circle->buffers[i], RS_BUFFER_LARGE_CAPACITY);
   }
   circle->size = 0;
   circle->offset = 0;
}

void rsBufferCircleDestroy(RSBufferCircle *circle) {
   for (size_t i = 0; i < circle->capacity; ++i) {
      rsBufferDestroy(&circle->buffers[i]);
   }
   rsFree(circle->buffers);
   rsClear(circle, sizeof(RSBufferCircle));
}

void rsBufferCircleClone(RSBufferCircle *circle, const RSBufferCircle *source) {
   for (size_t i = 0; i < circle->capacity; ++i) {
      if (i < source->size) {
         rsBufferClone(&circle->buffers[i], bufferCircleGet((RSBufferCircle *)source, i));
      } else {
         rsBufferClear(&circle->buffers[i]);
      }
   }
   circle->size = source->size;
   circle->offset = 0;
}

RSBuffer *rsBufferCircleAppend(RSBufferCircle *circle) {
   size_t index;
   if (circle->size < circle->capacity) {
      index = circle->size;
      ++circle->size;
   } else {
      index = circle->offset;
      circle->offset = (circle->offset + 1) % circle->capacity;
   }
   RSBuffer *buffer = bufferCircleGet(circle, index);
   rsBufferClear(buffer);
   return buffer;
}
