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

#ifndef RS_COMPRESS_H
#define RS_COMPRESS_H
#include "std.h"
#include "util/buffer.h"
#include "util/frame.h"
#include "config.h"
#include <jpeglib.h>

typedef struct RSCompressDestination {
   struct jpeg_destination_mgr jpeg;
   RSBuffer *buffer;
   size_t size;
} RSCompressDestination;

typedef struct RSCompress {
   struct jpeg_compress_struct jpeg;
   RSCompressDestination dest;
   struct jpeg_error_mgr error;
} RSCompress;

void rsCompressCreate(RSCompress *compress, const RSConfig *config);
void rsCompressDestroy(RSCompress *compress);
void rsCompress(RSCompress *compress, RSBuffer *buffer, const RSFrame *frame);

#endif
