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

#ifndef RS_SYSTEM_H
#define RS_SYSTEM_H
#include "../std.h"
#include "../util/frame.h"

typedef struct RSSystem {
   void *extra;
   void (*destroy)(struct RSSystem *system);
   void (*frameCreate)(RSFrame *frame, struct RSSystem *system);
   bool (*wantsSave)(struct RSSystem *system);
} RSSystem;

void rsSystemDestroy(RSSystem *system);
void rsSystemFrameCreate(RSFrame *frame, RSSystem *system);
bool rsSystemWantsSave(RSSystem *system);

#endif
