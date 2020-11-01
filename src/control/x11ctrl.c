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

typedef struct X11Control {
   RSControl control;
   RSXDisplay *display;
} X11Control;

#ifdef RS_BUILD_X11_FOUND
static void x11ControlGrabKey(X11Control *x11, int key, unsigned mods) {
   Window window = DefaultRootWindow(x11->display);
   XGrabKey(x11->display, key, mods, window, 0, GrabModeAsync, GrabModeAsync);
}
#endif

static void x11ControlDestroy(RSControl *control) {
#ifdef RS_BUILD_X11_FOUND
   X11Control *x11 = (X11Control *)control;
   if (x11->display != NULL) {
      XCloseDisplay(x11->display);
      x11->display = NULL;
   }
#else
   (void)control;
#endif
}

static int x11ControlWantsSave(RSControl *control) {
#ifdef RS_BUILD_X11_FOUND
   int ret = 0;
   X11Control *x11 = (X11Control *)control;
   while (XPending(x11->display) > 0) {
      XEvent event;
      XNextEvent(x11->display, &event);
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

int rsX11ControlCreate(RSControl **control) {
   int ret = 0;
   X11Control *x11 = av_mallocz(sizeof(X11Control));
   *control = &x11->control;
   if (x11 == NULL) {
      ret = AVERROR(ENOMEM);
      goto error;
   }

   x11->control.destroy = x11ControlDestroy;
   x11->control.wantsSave = x11ControlWantsSave;
   if ((ret = rsXDisplayOpen(&x11->display, NULL)) < 0) {
      goto error;
   }

#ifdef RS_BUILD_X11_FOUND
   int key = XKeysymToKeycode(x11->display, XK_R);
   unsigned mods = ControlMask | ShiftMask;
   x11ControlGrabKey(x11, key, mods);
   // Also allow capslock and numslock (Mod2) to be enabled
   x11ControlGrabKey(x11, key, mods | LockMask);
   x11ControlGrabKey(x11, key, mods | Mod2Mask);
   x11ControlGrabKey(x11, key, mods | LockMask | Mod2Mask);
#endif

   return 0;
error:
   rsControlDestroy(control);
   return ret;
}
