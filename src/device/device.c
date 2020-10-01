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

#include "device.h"
#include "x11.h"
#include <libavdevice/avdevice.h>

void rsDeviceInit(void) {
   avdevice_register_all();
}

int rsVideoDeviceCreate(RSDevice *device) {
   int ret;
   if ((ret = rsX11DeviceCreate(device)) >= 0) {
      return 0;
   }
   av_log(NULL, AV_LOG_WARNING, "Failed to create X11 device: %s\n", av_err2str(ret));

   return AVERROR(ENOSYS);
}
