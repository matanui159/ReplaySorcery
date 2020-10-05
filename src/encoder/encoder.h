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

#ifndef RS_ENCODER_H
#define RS_ENCODER_H
#include "../device/device.h"
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>

typedef struct RSEncoder {
   void *extra;
   void (*destroy)(struct RSEncoder *encoder);
   int (*getPacket)(struct RSEncoder *encoder, AVPacket *packet);
} RSEncoder;

static av_always_inline void rsEncoderDestroy(RSEncoder *encoder) {
   if (encoder->destroy != NULL) {
      encoder->destroy(encoder);
   }
}

static av_always_inline int rsEncoderGetPacket(RSEncoder *encoder, AVPacket *packet) {
   if (encoder->getPacket == NULL) {
      return AVERROR(ENOSYS);
   } else {
      encoder->getPacket(encoder, packet);
   }
}

int rsVideoEncoderCreate(RSEncoder *encoder, const RSDevice *input);

#endif
