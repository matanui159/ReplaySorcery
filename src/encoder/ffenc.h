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

#ifndef RS_ENCODER_FFENC_H
#define RS_ENCODER_FFENC_H
#include "encoder.h"

int rsFFmpegEncoderCreate(RSEncoder *encoder, const char *name, const char *filterFmt,
                          ...) av_printf_format(3, 4);
void rsFFmpegEncoderSetOption(RSEncoder *encoder, const char *key, const char *fmt, ...)
    av_printf_format(3, 4);
AVCodecContext *rsFFmpegEncoderGetContext(RSEncoder *encoder);
int rsFFmpegEncoderOpen(RSEncoder *encoder, const AVCodecParameters *params,
                        const AVBufferRef *hwFrames);

#endif
