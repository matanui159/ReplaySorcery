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
#include <libavutil/avutil.h>
#include <libavutil/frame.h>

typedef struct RSDevice {
   void *extra;
   void (*destroy)(struct RSDevice *device);
   int (*getFrame)(struct RSDevice *device, AVFrame *frame);
} RSDevice;

static av_always_inline void rsDeviceDestroy(RSDevice *device) {
   device->destroy(device);
}

static av_always_inline int rsDeviceGetFrame(RSDevice *device, AVFrame *frame) {
   return device->getFrame(device, frame);
}

#endif
