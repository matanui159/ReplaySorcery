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

#include "x11dev.h"
#include "../config.h"
#include "device.h"
#include "ffdev.h"

static int xclientError(int error) {
   switch (error) {
   case XCB_CONN_ERROR:
      return AVERROR(EPIPE);
   case XCB_CONN_CLOSED_EXT_NOTSUPPORTED:
      return AVERROR(ENOSYS);
   case XCB_CONN_CLOSED_MEM_INSUFFICIENT:
      return AVERROR(ENOMEM);
   case XCB_CONN_CLOSED_REQ_LEN_EXCEED:
      return AVERROR(ERANGE);
   case XCB_CONN_CLOSED_PARSE_ERR:
      return AVERROR(EPROTO);
   case XCB_CONN_CLOSED_INVALID_SCREEN:
      return AVERROR(EINVAL);
   default:
      return AVERROR_EXTERNAL;
   }
}

int rsXClientCreate(RSXClient *client, const char *display) {
#ifdef RS_BUILD_X11_FOUND
   int ret;
   client->xcb = xcb_connect(display, &client->screenIndex);
   if (client->xcb == NULL) {
      av_log(NULL, AV_LOG_ERROR, "Failed to connect to X11 server: Unknown error\n");
      ret = AVERROR_EXTERNAL;
      goto error;
   }
   if ((ret = xcb_connection_has_error(client->xcb)) != 0) {
      ret = xclientError(ret);
      av_log(NULL, AV_LOG_ERROR, "Failed to connect to X11 server: %s\n",
             av_err2str(ret));
      goto error;
   }

   return 0;
error:
   rsXClientDestroy(client);
   return ret;

#else
   (void)client;
   (void)display;
   av_log(NULL, AV_LOG_ERROR, "X11 and XCB was not found during compilation\n");
   return AVERROR(ENOSYS);
#endif
}

void rsXClientDestroy(RSXClient *client) {
#ifdef RS_BUILD_X11_FOUND
   if (client->xcb != NULL) {
      xcb_disconnect(client->xcb);
      client->xcb = NULL;
   }

#else
   (void)client;
#endif
}

const RSXScreen *rsXClientGetScreen(RSXClient *client, int screenIndex) {
#ifdef RS_BUILD_X11_FOUND
   const xcb_setup_t *setup = xcb_get_setup(client->xcb);
   xcb_screen_iterator_t roots = xcb_setup_roots_iterator(setup);
   for (int i = 0; i < screenIndex; ++i) {
      xcb_screen_next(&roots);
   }
   return roots.data;

#else
   (void)client;
   (void)screenIndex;
   return NULL;
#endif
}

int rsXClientGetKeyCode(RSXClient *client, uint32_t sym) {
#ifdef RS_BUILD_X11_FOUND
   xcb_generic_error_t *err;
   const xcb_setup_t *setup = xcb_get_setup(client->xcb);
   xcb_get_keyboard_mapping_cookie_t ckmapping =
       xcb_get_keyboard_mapping(client->xcb, setup->min_keycode,
                                (xcb_keycode_t)(setup->max_keycode - setup->min_keycode));
   xcb_get_keyboard_mapping_reply_t *mapping =
       xcb_get_keyboard_mapping_reply(client->xcb, ckmapping, &err);
   if (mapping == NULL) {
      av_log(NULL, AV_LOG_ERROR, "Failed to get keyboard mapping: %" PRIu8 "\n",
             err->error_code);
      return AVERROR_EXTERNAL;
   }

   xcb_keysym_t *keysyms = xcb_get_keyboard_mapping_keysyms(mapping);
   int length = xcb_get_keyboard_mapping_keysyms_length(mapping);
   int ret = AVERROR(EINVAL);
   for (int i = 0; i < length; ++i) {
      if (keysyms[i] == sym) {
         ret = i / mapping->keysyms_per_keycode + setup->min_keycode;
         break;
      }
   }
   free(mapping);
   return ret;

#else
   (void)client;
   (void)sym;
   return AVERROR(ENOSYS);
#endif
}

int rsX11DeviceCreate(RSDevice *device) {
   int ret;
   const char *deviceName = rsConfig.videoDevice;
   if (strcmp(deviceName, "auto") == 0) {
      deviceName = getenv("DISPLAY");
      if (deviceName == NULL) {
         deviceName = ":0";
      }
   }

   int width = rsConfig.videoWidth;
   int height = rsConfig.videoHeight;
   RSXClient client;
   if (rsXClientCreate(&client, deviceName) >= 0) {
#ifdef RS_BUILD_X11_FOUND
      const RSXScreen *screen = rsXClientGetScreen(&client, client.screenIndex);
      if (width == RS_CONFIG_AUTO) {
         width = screen->width_in_pixels - rsConfig.videoX;
      }
      if (height == RS_CONFIG_AUTO) {
         height = screen->height_in_pixels - rsConfig.videoY;
      }
      rsXClientDestroy(&client);
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

   rsFFmpegDeviceSetOption(device, "grab_x", "%i", rsConfig.videoX);
   rsFFmpegDeviceSetOption(device, "grab_y", "%i", rsConfig.videoY);
   rsFFmpegDeviceSetOption(device, "video_size", "%ix%i", width, height);
   rsFFmpegDeviceSetOption(device, "framerate", "%i", rsConfig.videoFramerate);
   if ((ret = rsFFmpegDeviceOpen(device, deviceName)) < 0) {
      goto error;
   }

   return 0;
error:
   rsDeviceDestroy(device);
   return ret;
}
