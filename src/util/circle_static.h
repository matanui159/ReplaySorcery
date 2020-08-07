/*
 * Copyright (C) 2020  Patryk Seregiet
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

#ifndef RS_UTIL_CIRCLE_STATIC_H
#define RS_UTIL_CIRCLE_STATIC_H
#include "../std.h"

typedef struct RSCircleStatic {
   uint8_t *data;
   size_t size;
   size_t index;
} RSCircleStatic;

void rsCircleStaticCreate(RSCircleStatic *circle, size_t size);
void rsCircleStaticDestroy(RSCircleStatic *circle);
void rsCircleStaticAdd(RSCircleStatic *circle, uint8_t *src, size_t size);
void rsCircleStaticGet(RSCircleStatic *circle, uint8_t *dst, size_t size);
void rsCircleStaticMoveBackIndex(RSCircleStatic *circle, size_t bytes);

#endif
