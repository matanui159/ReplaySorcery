/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef RS_UTIL_BUFFER_H
#define RS_UTIL_BUFFER_H
#include "../std.h"

typedef struct RSBuffer {
   char *data;
   size_t size;
   size_t capacity;
} RSBuffer;

void rsBufferCreate(RSBuffer *buffer);
void rsBufferDestroy(RSBuffer *buffer);
void *rsBufferGrow(RSBuffer *buffer, size_t size);
void *rsBufferAutoGrow(RSBuffer *buffer, size_t *size);
void rsBufferShrink(RSBuffer *buffer, size_t size);

#endif
