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

#ifndef RS_DEVICE_H
#define RS_DEVICE_H
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

typedef struct RSDevice {
   AVCodecParameters *params;
   AVBufferRef *hwFrames;
   void *extra;
   void (*destroy)(struct RSDevice *device);
   int (*nextFrame)(struct RSDevice *device, AVFrame *frame);
} RSDevice;

static av_always_inline int rsDeviceNextFrame(RSDevice *device, AVFrame *frame) {
   return device->nextFrame(device, frame);
}

int rsDeviceCreate(RSDevice *device);
void rsDeviceDestroy(RSDevice *device);

int rsX11DeviceCreate(RSDevice *device);
int rsKmsDeviceCreate(RSDevice *device, const char *deviceName, int framerate);
int rsKmsServiceDeviceCreate(RSDevice *device);
int rsVideoDeviceCreate(RSDevice *device);

// Services

#define RS_SERVICE_DEVICE_PATH "/tmp/replay-sorcery/device.sock"

typedef struct RSServiceDeviceInfo {
   int framerate;
   uint8_t deviceLength;
} RSServiceDeviceInfo;

#endif
