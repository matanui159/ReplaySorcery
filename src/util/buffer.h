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
