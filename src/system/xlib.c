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

#include "xlib.h"
#include "../util/log.h"
#include "../util/memory.h"
#include "../util/string.h"
#include "framerate.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include <X11/keysym.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>

typedef struct XlibSystemExtra {
   const RSConfig *config;
   Display *display;
   Window rootWindow;
   struct timespec frameTime;
   XShmSegmentInfo sharedInfo;
   XImage *sharedFrame;
} XlibSystemExtra;

static uint8_t xlibIgnore = Success;

static int xlibSystemError(Display *display, XErrorEvent *event) {
   char error[1024];
   XGetErrorText(display, event->error_code, error, sizeof(error));
   if (xlibIgnore != Success && event->error_code == xlibIgnore) {
      rsLog("X11 warning: %s", error);
   } else {
      rsError("X11 error: %s", error);
   }
   return 0;
}

static void xlibSystemGrabKey(XlibSystemExtra *extra, int key, unsigned mods) {
   XGrabKey(extra->display, key, mods, extra->rootWindow, false, GrabModeAsync,
            GrabModeAsync);
}

static void xlibSystemDestroy(RSSystem *system) {
   XlibSystemExtra *extra = system->extra;
   if (extra->sharedFrame != NULL) {
      XDestroyImage(extra->sharedFrame);
      XShmDetach(extra->display, &extra->sharedInfo);
      shmdt(extra->sharedInfo.shmaddr);
   }
   XCloseDisplay(extra->display);
   rsMemoryDestroy(extra);
}

static void xlibSystemFrameDestroy(RSFrame *frame) {
   if (frame->extra != NULL) {
      XImage *image = frame->extra;
      XDestroyImage(image);
   }
}

static void xlibSystemFrameImage(RSFrame *frame, XImage *image) {
   if (image->depth != 24 || image->bits_per_pixel != 32 ||
       image->byte_order != LSBFirst) {
      rsError("Only BGRX X11 images are supported");
   }
   frame->data = (uint8_t *)image->data;
   frame->width = (size_t)image->width;
   frame->height = (size_t)image->height;
   frame->strideX = 4;
   frame->strideY = (size_t)image->bytes_per_line;
   frame->destroy = xlibSystemFrameDestroy;
}

static void xlibSystemFrameCreate(RSFrame *frame, RSSystem *system) {
   XlibSystemExtra *extra = system->extra;
   rsFramerateSleep(&extra->frameTime, extra->config->framerate);
   // This sometimes returns BadMatch (Invalid parameter) during suspension or sleep
   xlibIgnore = BadMatch;
   if (extra->sharedFrame == NULL) {
      // Create a new image by getting it from X11
      XImage *image = XGetImage(extra->display, extra->rootWindow, extra->config->offsetX,
                                extra->config->offsetY, (unsigned)extra->config->width,
                                (unsigned)extra->config->height, AllPlanes, ZPixmap);
      if (image == NULL) {
         // Something went wrong but the error was ignored (see above), create a blank
         // frame
         rsFrameCreate(frame, (size_t)extra->config->width, (size_t)extra->config->height,
                       4);
      } else {
         xlibSystemFrameImage(frame, image);
         frame->extra = image;
      }
   } else {
      // Use shared memory to access the next frame
      XShmGetImage(extra->display, extra->rootWindow, extra->sharedFrame,
                   (int)extra->config->offsetX, (int)extra->config->offsetY, AllPlanes);
      xlibSystemFrameImage(frame, extra->sharedFrame);
      // We set this to null so the destructor does not free the image
      frame->extra = NULL;
   }
   xlibIgnore = Success;
}

static bool xlibSystemWantsSave(RSSystem *system) {
   XlibSystemExtra *extra = system->extra;
   bool save = false;
   while (XPending(extra->display) > 0) {
      XEvent event;
      XNextEvent(extra->display, &event);
      if (event.type == KeyPress) {
         save = true;
      }
   }
   return save;
}

bool rsXlibSystemCreate(RSSystem *system, RSConfig *config) {
   system->extra = rsMemoryCreate(sizeof(XlibSystemExtra));
   XlibSystemExtra *extra = system->extra;
   extra->config = config;
   extra->display = XOpenDisplay(NULL);
   if (extra->display == NULL) {
      rsLog("Failed to open X11 display");
      return false;
   }
   XSetErrorHandler(xlibSystemError);
   XSynchronize(extra->display, true);
   rsLog("X11 vendor: %s %i.%i.%i", ServerVendor(extra->display),
         ProtocolVersion(extra->display), ProtocolRevision(extra->display),
         VendorRelease(extra->display));
   extra->rootWindow = DefaultRootWindow(extra->display);

   Screen *screen = DefaultScreenOfDisplay(extra->display);
   int width = WidthOfScreen(screen);
   int height = HeightOfScreen(screen);
   // Handle automatic variables
   if (config->width == RS_CONFIG_AUTO) {
      config->width = width - config->offsetX;
   }
   if (config->height == RS_CONFIG_AUTO) {
      config->height = height - config->offsetY;
   }
   // Make sure the recording rectangle fits on the screen
   if (config->offsetX + config->width > width ||
       config->offsetY + config->height > height) {
      rsError("Recording rectangle does not fit X11 screen");
   }

   // Figure out the key and mods to use
   int key = 0;
   unsigned mods = 0;
   char *keyCombo = rsStringClone(config->keyCombo);
   char *keyString;
   while ((keyString = rsStringSplit(&keyCombo, '+'))) {
      keyString = rsStringTrim(keyString);
      if (keyCombo == NULL) {
         // This is the last split so it must be the key
         KeySym symbol = XStringToKeysym(keyString);
         if (symbol == NoSymbol) {
            rsError("Unknown key name '%s'", keyString);
         }
         key = XKeysymToKeycode(extra->display, symbol);
      } else if (strcmp(keyString, "Ctrl") == 0) {
         mods |= ControlMask;
      } else if (strcmp(keyString, "Shift") == 0) {
         mods |= ShiftMask;
      } else if (strcmp(keyString, "Alt") == 0) {
         mods |= Mod1Mask;
      } else if (strcmp(keyString, "Super") == 0) {
         mods |= Mod4Mask;
      } else {
         rsError("Unknown key modifier name '%s'", keyString);
      }
   }
   rsMemoryDestroy(keyCombo);
   xlibSystemGrabKey(extra, key, mods);
   // Also need to register when Capslock or Numlock (Mod2) is enabled
   xlibSystemGrabKey(extra, key, mods | LockMask);
   xlibSystemGrabKey(extra, key, mods | Mod2Mask);
   xlibSystemGrabKey(extra, key, mods | LockMask | Mod2Mask);

   int major, minor, pixmap;
   bool shared = XShmQueryVersion(extra->display, &major, &minor, &pixmap);
   if (shared) {
      rsLog("X11 shared memory version: %i.%i", major, minor);
      // Create the shared image
      extra->sharedFrame = XShmCreateImage(
          extra->display, DefaultVisualOfScreen(screen),
          (unsigned)DefaultDepthOfScreen(screen), ZPixmap, NULL, &extra->sharedInfo,
          (unsigned)config->width, (unsigned)config->height);

      // Create the shared memory ID
      extra->sharedInfo.shmid = shmget(
          IPC_PRIVATE,
          (size_t)(extra->sharedFrame->bytes_per_line * extra->sharedFrame->height),
          IPC_CREAT | 0777);

      // Attach the shared memory
      extra->sharedInfo.shmaddr = shmat(extra->sharedInfo.shmid, NULL, 0);
      extra->sharedFrame->data = extra->sharedInfo.shmaddr;
      extra->sharedInfo.readOnly = false;
      XShmAttach(extra->display, &extra->sharedInfo);

      // We do not need the shared memory ID anymore
      shmctl(extra->sharedInfo.shmid, IPC_RMID, NULL);
   } else {
      rsLog("X11 shared memory not supported");
      extra->sharedFrame = NULL;
   }

   clock_gettime(CLOCK_MONOTONIC, &extra->frameTime);
   system->destroy = xlibSystemDestroy;
   system->frameCreate = xlibSystemFrameCreate;
   system->wantsSave = xlibSystemWantsSave;
   return true;
}
