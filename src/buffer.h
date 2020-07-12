/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef RS_BUFFER_H
#define RS_BUFFER_H
#include "common.h"

#define RS_BUFFER_SMALL_CAPACITY 16
#define RS_BUFFER_LARGE_CAPACITY 1024

typedef struct RSBuffer {
   void *data;
   size_t size;
   size_t capacity;
} RSBuffer;

void rsBufferCreate(RSBuffer *buffer, size_t capacity);
void rsBufferDestroy(RSBuffer *buffer);
void rsBufferClone(RSBuffer *buffer, const RSBuffer *source);
void *rsBufferAppend(RSBuffer *buffer, size_t size);
void *rsBufferGetSpace(RSBuffer *buffer, size_t *size);
void rsBufferOptimize(RSBuffer *buffer);
void rsBufferClear(RSBuffer *buffer);

#endif
