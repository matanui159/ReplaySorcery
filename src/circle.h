/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef RS_CIRCLE_H
#define RS_CIRCLE_H
#include "buffer.h"
#include "common.h"
#include "config/config.h"

typedef struct RSBufferCircle {
   RSBuffer *buffers;
   size_t size;
   size_t capacity;
   size_t offset;
} RSBufferCircle;

void rsBufferCircleCreate(RSBufferCircle *circle, const RSConfig *config);
void rsBufferCircleDestroy(RSBufferCircle *circle);
void rsBufferCircleClone(RSBufferCircle *circle, const RSBufferCircle *source);
RSBuffer *rsBufferCircleAppend(RSBufferCircle *circle);

#endif
