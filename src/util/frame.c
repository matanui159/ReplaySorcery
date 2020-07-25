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
#include "log.h"
#include "memory.h"

#define FRAME_GET(frame, x, y)                                                           \
   ((frame)->data + (y) * (frame)->strideY + (x) * (frame)->strideX)

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

void rsFrameConvertI420(const RSFrame *restrict frame, RSFrame *restrict yFrame,
                        RSFrame *restrict uFrame, RSFrame *restrict vFrame) {
   // We can assume frame is divisible by 2 due to check done in `config.c`

   for (size_t y = 0; y < frame->height; y += 2) {
      uint16_t uRow[frame->width];
      uint16_t vRow[frame->width];

      // Copy the first Y-row and add together the top half of the U and V pixels
      for (size_t x = 0; x < frame->width; x += 2) {
         const uint8_t *restrict leftPixel = FRAME_GET(frame, x, y);
         const uint8_t *restrict rightPixel = FRAME_GET(frame, x + 1, y);
         *FRAME_GET(yFrame, x, y) = leftPixel[0];
         *FRAME_GET(yFrame, x + 1, y) = rightPixel[0];
         uRow[x >> 1] = (uint16_t)(leftPixel[1] + rightPixel[1]);
         vRow[x >> 1] = (uint16_t)(leftPixel[2] + rightPixel[2]);
      }

      // Copy the second Y-row and calculate the full U and V pixels
      for (size_t x = 0; x < frame->width; x += 2) {
         const uint8_t *restrict leftPixel = FRAME_GET(frame, x, y + 1);
         const uint8_t *restrict rightPixel = FRAME_GET(frame, x + 1, y + 1);
         *FRAME_GET(yFrame, x, y + 1) = leftPixel[0];
         *FRAME_GET(yFrame, x + 1, y + 1) = rightPixel[0];
         uint8_t uPixel = (uint8_t)((uRow[x >> 1] + leftPixel[1] + rightPixel[1]) >> 2);
         uint8_t vPixel = (uint8_t)((vRow[x >> 1] + leftPixel[2] + rightPixel[2]) >> 2);
         *FRAME_GET(uFrame, x >> 1, y >> 1) = uPixel;
         *FRAME_GET(vFrame, x >> 1, y >> 1) = vPixel;
      }
   }
}
