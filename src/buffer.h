/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef RS_BUFFER_H
#define RS_BUFFER_H
#include "common.h"

typedef struct RSBuffer {
   void *data;
   size_t size;
   size_t capacity;
} RSBuffer;

void rsBufferCreate(RSBuffer *buffer, size_t capacity);
void rsBufferDestroy(RSBuffer *buffer);
void *rsBufferAppend(RSBuffer *buffer, size_t size);
void *rsBufferGetSpace(RSBuffer *buffer, size_t *size);
void rsBufferClear(RSBuffer *buffer);

#endif
