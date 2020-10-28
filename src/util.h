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

#ifndef RS_UTIL_H
#define RS_UTIL_H
#include <libavutil/avutil.h>
#include <libavutil/dict.h>
#include "rsbuild.h"
#ifdef RS_BUILD_X11_FOUND
#include <X11/Xlib.h>
#endif

#ifdef RS_BUILD_X11_FOUND
typedef Display RSXDisplay;
#else
typedef char RSXDisplay;
#endif

char *rsFormat(const char *fmt, ...);
char *rsFormatv(const char *fmt, va_list args);

void rsOptionsSet(AVDictionary **options, int *error, const char *key, const char *fmt, ...);
void rsOptionsSetv(AVDictionary **options, int *error, const char *key, const char *fmt, va_list ars);
void rsOptionsDestroy(AVDictionary **options);

int rsXDisplayOpen(RSXDisplay **display, const char *name);

#endif
