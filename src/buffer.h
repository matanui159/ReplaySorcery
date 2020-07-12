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

void rsCreateBuffer(RSBuffer *buffer, size_t capacity);
void rsDestroyBuffer(RSBuffer *buffer);
void *rsAppendBuffer(RSBuffer *buffer, size_t size);
void *rsGetBufferSpace(RSBuffer *buffer, size_t *size);
void rsClearBuffer(RSBuffer *buffer);

#endif
