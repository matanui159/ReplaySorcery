/*
 * Copyright (C) 2020  Joshua Minter
 *
 * This file is part of ReplaySorcery.
 *
 * ReplaySorcery is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ReplaySorcery is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ReplaySorcery.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "frame.h"
#include "memory.h"

static void frameDestroy(RSFrame *frame) {
   rsMemoryDestroy(frame->data);
}

static void frameConvertChroma(const RSFrame *frame, RSFrame *chroma, size_t offset) {
   for (size_t y = 0; y < chroma->height; ++y) {
      for (size_t x = 0; x < chroma->width; ++x) {
         uint8_t c0 = rsFrameGet(frame, x * 2, y * 2)[offset];
         uint8_t c1 = rsFrameGet(frame, x * 2 + 1, y * 2)[offset];
         uint8_t c2 = rsFrameGet(frame, x * 2, y * 2 + 1)[offset];
         uint8_t c3 = rsFrameGet(frame, x * 2 + 1, y * 2 + 1)[offset];
         *rsFrameGet(chroma, x, y) = (uint8_t)((c0 + c1 + c2 + c3) / 4);
      }
   }
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

void rsFrameConvertI420(const RSFrame *frame, RSFrame *yFrame, RSFrame *uFrame,
                        RSFrame *vFrame) {
   // TODO: this is very slow
   for (size_t y = 0; y < yFrame->height; ++y) {
      for (size_t x = 0; x < yFrame->width; ++x) {
         *rsFrameGet(yFrame, x, y) = *rsFrameGet(frame, x, y);
      }
   }
   frameConvertChroma(frame, uFrame, 1);
   frameConvertChroma(frame, vFrame, 2);
}
