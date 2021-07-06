/*
 * Copyright (C) 2020-2021  Joshua Minter
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
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

typedef struct RSEncoder {
   AVCodecParameters *params;
   void *extra;
   void (*destroy)(struct RSEncoder *encoder);
   int (*sendFrame)(struct RSEncoder *encoder, AVFrame *frame);
   int (*nextPacket)(struct RSEncoder *encoder, AVPacket *packet);
} RSEncoder;

static av_always_inline int rsEncoderSendFrame(RSEncoder *encoder, AVFrame *frame) {
   return encoder->sendFrame(encoder, frame);
}

static av_always_inline int rsEncoderNextPacket(RSEncoder *encoder, AVPacket *packet) {
   return encoder->nextPacket(encoder, packet);
}

int rsEncoderCreate(RSEncoder *encoder);
void rsEncoderDestroy(RSEncoder *encoder);

int rsX264EncoderCreate(RSEncoder *encoder, const AVCodecParameters *params);
int rsOpenH264EncoderCreate(RSEncoder *encoder, const AVCodecParameters *params);
int rsX265EncoderCreate(RSEncoder *encoder, const AVCodecParameters *params);
int rsVaapiH264EncoderCreate(RSEncoder *encoder, const AVCodecParameters *params,
                             const AVBufferRef *hwFrames);
int rsVaapiHevcEncoderCreate(RSEncoder *encoder, const AVCodecParameters *params,
                             const AVBufferRef *hwFrames);
int rsVideoEncoderCreate(RSEncoder *encoder, const AVCodecParameters *params,
                         const AVBufferRef *hwFrames);

#endif
