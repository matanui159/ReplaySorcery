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

#include "../util.h"
#include "control.h"
#include "rsbuild.h"
#ifdef RS_BUILD_X11_FOUND
#include <X11/keysym.h>
#endif

#ifdef RS_BUILD_X11_FOUND
static void x11ControlGrabKey(RSXDisplay *display, int key, unsigned mods) {
   Window window = DefaultRootWindow(display);
   XGrabKey(display, key, mods, window, 0, GrabModeAsync, GrabModeAsync);
}
#endif

static void x11ControlDestroy(RSControl *control) {
#ifdef RS_BUILD_X11_FOUND
   RSXDisplay *display = control->extra;
   if (display != NULL) {
      XCloseDisplay(display);
      control->extra = NULL;
   }
#else
   (void)control;
#endif
}

static int x11ControlWantsSave(RSControl *control) {
#ifdef RS_BUILD_X11_FOUND
   int ret = 0;
   RSXDisplay *display = control->extra;
   while (XPending(display) > 0) {
      XEvent event;
      XNextEvent(display, &event);
      if (event.type == KeyPress) {
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
   RSXDisplay *display = NULL;
   ret = rsXDisplayOpen(&display, NULL);
   control->extra = display;
   control->destroy = x11ControlDestroy;
   control->wantsSave = x11ControlWantsSave;
   if (ret < 0) {
      goto error;
   }

#ifdef RS_BUILD_X11_FOUND
   int key = XKeysymToKeycode(display, XK_R);
   unsigned mods = ControlMask | ShiftMask;
   x11ControlGrabKey(display, key, mods);
   // Also allow capslock and numslock (Mod2) to be enabled
   x11ControlGrabKey(display, key, mods | LockMask);
   x11ControlGrabKey(display, key, mods | Mod2Mask);
   x11ControlGrabKey(display, key, mods | LockMask | Mod2Mask);
#endif

   return 0;
error:
   rsControlDestroy(control);
   return ret;
}
