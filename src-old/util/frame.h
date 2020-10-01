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
void rsFrameConvertI420(const RSFrame *frame, RSFrame *yFrame, RSFrame *uFrame,
                        RSFrame *vFrame);

#endif
