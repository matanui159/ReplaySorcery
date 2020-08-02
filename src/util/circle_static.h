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
   int size;
   int index;
} RSCircleStatic;

void rsCircleStaticCreate(RSCircleStatic *circle, int size);
void rsCircleStaticDestroy(RSCircleStatic *circle);
void rsCircleStaticDuplicate(RSCircleStatic *dst, const RSCircleStatic *src);
void rsCircleStaticAdd(RSCircleStatic *circle, uint8_t *src, int size);
void rsCircleStaticGet(RSCircleStatic *circle, uint8_t *dst, int size);
void rsCircleStaticMoveBackIndex(RSCircleStatic *circle, int bytes);

#endif
