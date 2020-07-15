/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef RS_UTIL_CIRCLE_H
#define RS_UTIL_CIRCLE_H
#include "../std.h"
#include "buffer.h"

typedef struct RSBufferCircle {
   RSBuffer *buffers;
   size_t size;
   size_t capacity;
   size_t offset;
} RSBufferCircle;

void rsBufferCircleCreate(RSBufferCircle *circle, size_t capacity);
void rsBufferCircleDestroy(RSBufferCircle *circle);
RSBuffer *rsBufferCircleNext(RSBufferCircle *circle);
void rsBufferCircleExtract(RSBufferCircle *circle, RSBuffer *target);

#endif
