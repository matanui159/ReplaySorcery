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

#include "control.h"
#include "rsbuild.h"
#ifdef RS_BUILD_X11_FOUND
#include <X11/Xlib.h>
#include <X11/keysym.h>
#endif

typedef struct X11Control {
   Display *display;
} X11Control;

static void x11ControlGrabKey(RSControl *control, int key, unsigned mods) {
   X11Control *x11 = control->extra;
   Window window = DefaultRootWindow(x11->display);
   XGrabKey(x11->display, key, mods, window, 0, GrabModeAsync, GrabModeAsync);
}

static void x11ControlDestroy(RSControl *control) {
   X11Control *x11 = control->extra;
   if (x11 != NULL && x11->display != NULL) {
      XCloseDisplay(x11->display);
      x11->display = NULL;
   }
   av_freep(&control->extra);
}

static int x11ControlWantsSave(RSControl *control) {
   int ret = 0;
   X11Control *x11 = control->extra;
   while (XPending(x11->display) > 0) {
      XEvent event;
      XNextEvent(x11->display, &event);
      if (event.type == KeyPress) {
         ret = 1;
      }
   }
   return ret;
}

int rsX11ControlCreate(RSControl *control) {
   int ret = 0;
#ifdef RS_BUILD_X11_FOUND
   X11Control *x11 = av_mallocz(sizeof(X11Control));
   control->extra = x11;
   control->destroy = x11ControlDestroy;
   control->wantsSave = x11ControlWantsSave;
   if (x11 == NULL) {
      ret = AVERROR(ENOMEM);
      goto error;
   }

   x11->display = XOpenDisplay(NULL);
   if (x11->display == NULL) {
      av_log(NULL, AV_LOG_ERROR, "Failed to open X11 display\n");
      ret = AVERROR_EXTERNAL;
      goto error;
   }

   int key = XKeysymToKeycode(x11->display, XK_R);
   unsigned mods = ControlMask | ShiftMask;
   x11ControlGrabKey(control, key, mods);
   // Also allow capslock and numslock (Mod2) to be enabled
   x11ControlGrabKey(control, key, mods | LockMask);
   x11ControlGrabKey(control, key, mods | Mod2Mask);
   x11ControlGrabKey(control, key, mods | LockMask | Mod2Mask);

#else
   (void)control;
   av_log(NULL, AV_LOG_ERROR, "X11 was not found during compilation\n");
   ret = AVERROR(ENOSYS);
   goto error;
#endif

   return 0;
error:
   rsControlDestroy(control);
   return ret;
}
