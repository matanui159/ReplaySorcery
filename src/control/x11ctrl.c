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

#include "../config.h"
#include "../device/x11dev.h"
#include "control.h"
#include "rsbuild.h"
#ifdef RS_BUILD_X11_FOUND
#include <X11/Xlib.h>
#endif

#define X11_CONTROL_MASK_CAPSLOCK XCB_MOD_MASK_LOCK
#define X11_CONTROL_MASK_ALT XCB_MOD_MASK_1
#define X11_CONTROL_MASK_NUMLOCK XCB_MOD_MASK_2
#define X11_CONTROL_MASK_SUPER XCB_MOD_MASK_4

#ifdef RS_BUILD_X11_FOUND
static void x11ControlGrabKey(RSXClient *client, int key, uint16_t mods, int *ret) {
   if (*ret < 0) {
      return;
   }

   xcb_generic_error_t *err;
   const RSXScreen *screen = rsXClientGetScreen(client, client->screenIndex);
   xcb_void_cookie_t ckvoid =
       xcb_grab_key_checked(client->xcb, 0, screen->root, mods, (xcb_keycode_t)key,
                            XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
   if ((err = xcb_request_check(client->xcb, ckvoid)) != NULL) {
      av_log(NULL, AV_LOG_ERROR, "Failed to grab key: %" PRIu8 "\n", err->error_code);
      *ret = AVERROR_EXTERNAL;
   }
}
#endif

static void x11ControlDestroy(RSControl *control) {
#ifdef RS_BUILD_X11_FOUND
   RSXClient *client = control->extra;
   if (client != NULL) {
      rsXClientDestroy(client);
      av_freep(&control->extra);
   }

#else
   (void)control;
#endif
}

static int x11ControlWantsSave(RSControl *control) {
#ifdef RS_BUILD_X11_FOUND
   int ret = 0;
   RSXClient *client = control->extra;
   xcb_generic_event_t *event;
   while ((event = xcb_poll_for_event(client->xcb)) != NULL) {
      if (event->response_type == XCB_KEY_PRESS) {
         ret = 1;
      }
   }
   return ret;

#else
   (void)control;
   return AVERROR(ENOSYS);
#endif
}

int rsX11ControlCreate(RSControl *control) {
   int ret = 0;
   RSXClient *client = av_mallocz(sizeof(RSXClient));
   control->extra = client;
   control->destroy = x11ControlDestroy;
   control->wantsSave = x11ControlWantsSave;
   if (ret < 0) {
      goto error;
   }
   if ((ret = rsXClientCreate(client, NULL)) < 0) {
      goto error;
   }

#ifdef RS_BUILD_X11_FOUND
   int key = rsXClientGetKeyCode(client, (uint32_t)XStringToKeysym(rsConfig.keyName));
   if (key < 0) {
      ret = key;
      goto error;
   }

   uint16_t mods = 0;
   if (rsConfig.keyMods & RS_CONFIG_KEYMOD_CTRL) {
      mods |= XCB_MOD_MASK_CONTROL;
   }
   if (rsConfig.keyMods & RS_CONFIG_KEYMOD_SHIFT) {
      mods |= XCB_MOD_MASK_SHIFT;
   }
   if (rsConfig.keyMods & RS_CONFIG_KEYMOD_ALT) {
      mods |= X11_CONTROL_MASK_ALT;
   }
   if (rsConfig.keyMods & RS_CONFIG_KEYMOD_SUPER) {
      mods |= X11_CONTROL_MASK_SUPER;
   }

   x11ControlGrabKey(client, key, mods, &ret);
   // Also allow capslock and numslock to be enabled
   x11ControlGrabKey(client, key, mods | X11_CONTROL_MASK_CAPSLOCK, &ret);
   x11ControlGrabKey(client, key, mods | X11_CONTROL_MASK_NUMLOCK, &ret);
   x11ControlGrabKey(client, key,
                     mods | X11_CONTROL_MASK_CAPSLOCK | X11_CONTROL_MASK_NUMLOCK, &ret);
   if (ret < 0) {
      goto error;
   }
#endif

   return 0;
error:
   rsControlDestroy(control);
   return ret;
}
