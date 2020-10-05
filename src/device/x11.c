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

#include "x11.h"
#include "../config.h"
#include "demux.h"
#include "rsbuild.h"
#include <libavutil/avstring.h>
#include <libavutil/avutil.h>
#include <libavutil/dict.h>

#ifdef RS_BUILD_X11_FOUND
#include <X11/Xlib.h>
#endif

int rsX11DeviceCreate(RSDevice *device) {
   const char *display = getenv("DISPLAY");
   if (display == NULL) {
      display = ":0";
   }

   int width = rsConfig.videoWidth;
   int height = rsConfig.videoHeight;
#ifdef RS_BUILD_X11_FOUND
   if (width == RS_CONFIG_AUTO || height == RS_CONFIG_AUTO) {
      Display *connection = XOpenDisplay(display);
      if (connection == NULL) {
         av_log(NULL, AV_LOG_WARNING, "Failed to open X11 display\n");
      } else {
         av_log(NULL, AV_LOG_INFO, "X11 version: %i.%i\n", ProtocolVersion(connection),
                ProtocolRevision(connection));
         av_log(NULL, AV_LOG_INFO, "X11 vendor: %s v%i\n", ServerVendor(connection),
                VendorRelease(connection));
         Screen *screen = DefaultScreenOfDisplay(connection);
         if (width == RS_CONFIG_AUTO) {
            width = WidthOfScreen(screen) - rsConfig.videoX;
         }
         if (height == RS_CONFIG_AUTO) {
            height = HeightOfScreen(screen) - rsConfig.videoY;
         }
         XCloseDisplay(connection);
         av_log(NULL, AV_LOG_INFO, "Video size: %ix%i\n", width, height);
      }
   }

#else
   av_log(NULL, AV_LOG_WARNING, "X11 was not found during compilation\n");
#endif
   if (width == RS_CONFIG_AUTO || height == RS_CONFIG_AUTO) {
      av_log(NULL, AV_LOG_ERROR, "Could not detect X11 display size\n");
      return AVERROR(ENOSYS);
   }

   char *size = av_asprintf("%ix%i", width, height);
   if (size == NULL) {
      return AVERROR(ENOMEM);
   }

   AVDictionary *options = NULL;
   av_dict_set_int(&options, "grab_x", rsConfig.videoX, 0);
   av_dict_set_int(&options, "grab_y", rsConfig.videoY, 0);
   av_dict_set(&options, "video_size", size, AV_DICT_DONT_STRDUP_VAL);
   av_dict_set_int(&options, "framerate", rsConfig.videoFramerate, 0);
   if (av_dict_count(options) != 4) {
      av_dict_free(&options);
      return AVERROR(ENOMEM);
   }
   return rsDemuxDeviceCreate(device, "x11grab", display, &options);
}
