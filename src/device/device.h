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

#ifndef RS_DEVICE_H
#define RS_DEVICE_H
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/dict.h>

typedef struct RSDeviceParams {
   const char *name;
   const AVDictionary *options;
   const char *input;
} RSDeviceParams;

typedef struct RSDevice {
   AVFormatContext *formatCtx;
   AVStream *stream;
   AVCodecContext *codecCtx;
   AVPacket packet;
} RSDevice;

#define RS_DEVICE_INIT                                                                   \
   { NULL }

void rsDeviceInit(void);

int rsDeviceCreate(RSDevice *device, const RSDeviceParams *params);
void rsDeviceDestroy(RSDevice *device);
int rsDeviceGetFrame(RSDevice *device, AVFrame *frame);

int rsX11DeviceCreate(RSDevice *device);
int rsVideoDeviceCreate(RSDevice *device);

#endif
