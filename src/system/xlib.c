/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "xlib.h"
#include "../util/log.h"
#include "../util/memory.h"
#include "framerate.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include <X11/keysym.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>

typedef struct XlibSystemExtra {
   RSConfig config;
   Display *display;
   Window rootWindow;
   struct timespec frameTime;
   XShmSegmentInfo sharedInfo;
   XImage *sharedFrame;
} XlibSystemExtra;

static int xlibSystemError(Display *display, XErrorEvent *event) {
   char error[1024];
   XGetErrorText(display, event->error_code, error, sizeof(error));
   rsError("X11 error: %s", error);
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
      shmctl(extra->sharedInfo.shmid, IPC_RMID, NULL);
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

static void xlibSystemCreateFrame(RSFrame *frame, RSSystem *system) {
   XlibSystemExtra *extra = system->extra;
   rsFramerateSleep(&extra->frameTime, extra->config.framerate);
   XImage *image;
   if (extra->sharedFrame == NULL) {
      image = XGetImage(extra->display, extra->rootWindow, extra->config.offsetY,
                        extra->config.offsetY, (unsigned)extra->config.width,
                        (unsigned)extra->config.height, AllPlanes, ZPixmap);
      frame->extra = image;
   } else {
      XShmGetImage(extra->display, extra->rootWindow, extra->sharedFrame,
                   (int)extra->config.offsetX, (int)extra->config.offsetY, AllPlanes);
      image = extra->sharedFrame;
      frame->extra = NULL;
   }

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

bool rsXlibSystemCreate(RSSystem *system, const RSConfig *config) {
   system->extra = rsMemoryCreate(sizeof(XlibSystemExtra));
   XlibSystemExtra *extra = system->extra;
   extra->config = *config;
   extra->display = XOpenDisplay(NULL);
   if (extra->display == NULL) {
      rsLog("Failed to open X11 display");
      return false;
   }
   XSetErrorHandler(xlibSystemError);
   rsLog("X11 vendor: %s %i.%i.%i", ServerVendor(extra->display),
         ProtocolVersion(extra->display), ProtocolRevision(extra->display),
         VendorRelease(extra->display));
   extra->rootWindow = DefaultRootWindow(extra->display);

   int key = XKeysymToKeycode(extra->display, XK_R);
   // Ctrl and Super (Mod4) keys
   unsigned mods = ControlMask | Mod4Mask;
   xlibSystemGrabKey(extra, key, mods);
   // Also need to register when Capslock or Numlock (Mod2) is enabled
   xlibSystemGrabKey(extra, key, mods | LockMask);
   xlibSystemGrabKey(extra, key, mods | Mod2Mask);
   xlibSystemGrabKey(extra, key, mods | LockMask | Mod2Mask);

   int major, minor, pixmap;
   bool shared = XShmQueryVersion(extra->display, &major, &minor, &pixmap);
   if (shared) {
      rsLog("X11 shared memory version: %i.%i", major, minor);
      int screen = DefaultScreen(extra->display);

      // Create the shared image
      extra->sharedFrame = XShmCreateImage(extra->display, DefaultVisual(extra->display, screen), (unsigned)DefaultDepth(extra->display, screen), ZPixmap, NULL, &extra->sharedInfo, (unsigned)config->width, (unsigned)config->height);

      // Create the shared memory ID
      extra->sharedInfo.shmid = shmget(
          IPC_PRIVATE,
          (size_t)(extra->sharedFrame->bytes_per_line * extra->sharedFrame->height),
          IPC_CREAT | 0777);

      // Attach the shared memory
      extra->sharedInfo.shmaddr = shmat(extra->sharedInfo.shmid, NULL, 0);
      extra->sharedFrame->data = extra->sharedInfo.shmaddr;
      XShmAttach(extra->display, &extra->sharedInfo);
   } else {
      rsLog("X11 shared memory not supported");
      extra->sharedFrame = NULL;
   }

   clock_gettime(CLOCK_MONOTONIC, &extra->frameTime);
   system->destroy = xlibSystemDestroy;
   system->frameCreate = xlibSystemCreateFrame;
   system->wantsSave = xlibSystemWantsSave;
   return true;
}
