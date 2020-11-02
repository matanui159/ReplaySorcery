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

#include "../config.h"
#include "../util.h"
#include "device.h"
#include "ffdev.h"
#include "rsbuild.h"

int rsX11DeviceCreate(RSDevice **device) {
   int ret;
   const char *input = getenv("DISPLAY");
   if (input == NULL) {
      input = ":0";
   }

   int width = rsConfig.videoWidth;
   int height = rsConfig.videoHeight;
   RSXDisplay *display;
   if (rsXDisplayOpen(&display, input) >= 0) {
#ifdef RS_BUILD_X11_FOUND
      Screen *screen = DefaultScreenOfDisplay(display);
      if (width == RS_CONFIG_AUTO) {
         width = WidthOfScreen(screen) - rsConfig.videoX;
      }
      if (height == RS_CONFIG_AUTO) {
         height = HeightOfScreen(screen) - rsConfig.videoY;
      }
      XCloseDisplay(display);
#endif
   }
   if (width == RS_CONFIG_AUTO || height == RS_CONFIG_AUTO) {
      av_log(NULL, AV_LOG_ERROR, "Could not detect X11 display size\n");
      ret = AVERROR(ENOSYS);
      goto error;
   }
   if ((ret = rsFFmpegDeviceCreate(device, "x11grab")) < 0) {
      goto error;
   }

   rsFFmpegDeviceOption(*device, "grab_x", "%i", rsConfig.videoX);
   rsFFmpegDeviceOption(*device, "grab_y", "%i", rsConfig.videoY);
   rsFFmpegDeviceOption(*device, "video_size", "%ix%i", width, height);
   rsFFmpegDeviceOption(*device, "framerate", "%i", rsConfig.videoFramerate);
   if ((ret = rsFFmpegDeviceOpen(*device, input)) < 0) {
      goto error;
   }

   return 0;
error:
   rsDeviceDestroy(device);
   return ret;
}
