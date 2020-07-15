/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef RS_UTIL_FRAME_H
#define RS_UTIL_FRAME_H
#include "../std.h"

typedef struct RSFrame {
   uint8_t *data;
   size_t width;
   size_t height;
   size_t strideX;
   size_t strideY;
   void *extra;
   void (*destroy)(struct RSFrame *frame);
} RSFrame;

void rsFrameCreate(RSFrame *frame, size_t width, size_t height, size_t strideX);
void rsFrameDestroy(RSFrame *frame);

#endif
