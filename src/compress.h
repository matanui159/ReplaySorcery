/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef RS_COMPRESS_H
#define RS_COMPRESS_H
#include "common.h"
#include "system/system.h"
#include "buffer.h"
#include "config/config.h"
#include <stdio.h>
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

void rsCreateCompress(RSCompress *compress, const RSConfig *config);
void rsDestroyCompress(RSCompress *compress);
void rsCompressSystemFrame(RSCompress *compress, RSBuffer *buffer, RSSystemFrame *frame);

#endif
