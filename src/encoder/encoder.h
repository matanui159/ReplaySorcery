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
#include <libavutil/dict.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

typedef struct RSEncoderParams {
   const char *name;
   const AVDictionary *options;
   RSDevice *input;
   enum AVPixelFormat swFormat;
} RSEncoderParams;

typedef struct RSEncoder {
   RSDevice *input;
   AVCodecContext *codecCtx;
   AVFrame *frame;
   struct SwsContext *scaleCtx;
   AVFrame *scaleFrame;
} RSEncoder;

#define RS_ENCODER_INIT { NULL }

int rsEncoderCreate(RSEncoder *encoder, const RSEncoderParams *params);
void rsEncoderDestroy(RSEncoder *encoder);
int rsEncoderGetPacket(RSEncoder *encoder, AVPacket *packet);

int rsX264EncoderCreate(RSEncoder *encoder, RSDevice *input);
int rsVideoEncoderCreate(RSEncoder *encoder, RSDevice *input);

#endif
