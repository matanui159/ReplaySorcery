/*
 * Copyright (C) 2021  Joshua Minter
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

#ifndef RS_DEVICE_XCB_H
#define RS_DEVICE_XCB_H
#include "rsbuild.h"
#ifdef RS_BUILD_X11_FOUND
#include <xcb/xcb.h>
#endif

typedef struct RSXClient {
#ifdef RS_BUILD_X11_FOUND
   xcb_connection_t *xcb;
#endif
   int screenIndex;
} RSXClient;

#ifdef RS_BUILD_X11_FOUND
typedef xcb_screen_t RSXScreen;
#else
typedef char RSXScreen;
#endif

int rsXClientCreate(RSXClient *client, const char *display);
void rsXClientDestroy(RSXClient *client);
const RSXScreen *rsXClientGetScreen(RSXClient *client, int screenIndex);
int rsXClientGetKeyCode(RSXClient *client, uint32_t sym);

#endif
