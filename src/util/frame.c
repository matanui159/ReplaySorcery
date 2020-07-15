/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "frame.h"
#include "memory.h"

static void frameDestroy(RSFrame *frame) {
   rsMemoryDestroy(frame->data);
}

void rsFrameCreate(RSFrame *frame, size_t width, size_t height, size_t strideX) {
   frame->strideY = width * strideX;
   frame->data = rsMemoryCreate(frame->strideY * height);
   frame->width = width;
   frame->height = height;
   frame->strideX = strideX;
   frame->destroy = frameDestroy;
}

void rsFrameDestroy(RSFrame *frame) {
   frame->destroy(frame);
   rsMemoryClear(frame, sizeof(RSFrame));
}
