/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

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
