#include "xlib.h"
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
   XImage *frame;
   struct timespec frameTime;
   XShmSegmentInfo sharedInfo;
   bool shared;
} XlibSystemExtra;

static int xlibSystemError(Display *display, XErrorEvent *event) {
   char error[1024];
   XGetErrorText(display, event->error_code, error, sizeof(error));
   rsError(0, "X11 error: %s", error);
}

static void xlibSystemGrabKey(XlibSystemExtra *extra, int key, unsigned mods) {
   XGrabKey(extra->display, key, mods, extra->rootWindow, false, GrabModeAsync,
            GrabModeAsync);
}

static void xlibSystemDestroy(RSSystem *system) {
   XlibSystemExtra *extra = system->extra;
   if (extra->frame != NULL) {
      XDestroyImage(extra->frame);
   }
   XCloseDisplay(extra->display);
   rsFree(extra);
}

static void xlibSystemGetFrame(RSSystem *system, RSSystemFrame *frame) {
   XlibSystemExtra *extra = system->extra;
   rsFramerateSleep(&extra->frameTime, extra->config.framerate);
   if (extra->shared) {
      XShmGetImage(extra->display, extra->rootWindow, extra->frame,
                   (int)extra->config.offsetX, (int)extra->config.offsetY, AllPlanes);
   } else {
      if (extra->frame != NULL) {
         XDestroyImage(extra->frame);
      }
      extra->frame = XGetImage(extra->display, extra->rootWindow, extra->config.offsetY,
                               extra->config.offsetY, (unsigned)extra->config.width,
                               (unsigned)extra->config.height, AllPlanes, ZPixmap);
   }

   if (extra->frame->depth != 24 || extra->frame->bits_per_pixel != 32 ||
       extra->frame->byte_order != LSBFirst) {
      rsError(0, "Only BGRX X11 images are supported");
   }
   frame->data = (uint8_t *)extra->frame->data;
   frame->width = (size_t)extra->frame->width;
   frame->height = (size_t)extra->frame->height;
   frame->stride = (size_t)extra->frame->bytes_per_line;
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
   system->extra = rsAllocate(sizeof(XlibSystemExtra));
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
   unsigned mods = ControlMask | Mod4Mask;
   xlibSystemGrabKey(extra, key, mods);
   xlibSystemGrabKey(extra, key, mods | LockMask);
   xlibSystemGrabKey(extra, key, mods | Mod2Mask);
   xlibSystemGrabKey(extra, key, mods | LockMask | Mod2Mask);

   int major, minor, pixmaps;
   extra->shared = XShmQueryVersion(extra->display, &major, &minor, &pixmaps);
   if (!pixmaps) {
      extra->shared = false;
   }
   if (extra->shared) {
      rsLog("X11 shared memory version: %i.%i", major, minor);
      rsClear(&extra->sharedInfo, sizeof(XShmSegmentInfo));
      int screen = DefaultScreen(extra->display);
      extra->frame = XShmCreateImage(
          extra->display, DefaultVisual(extra->display, screen),
          (unsigned)DefaultDepth(extra->display, screen), ZPixmap, NULL,
          &extra->sharedInfo, (unsigned)config->width, (unsigned)config->height);
      extra->sharedInfo.shmid = shmget(
          IPC_PRIVATE, (size_t)(extra->frame->bytes_per_line * extra->frame->height),
          IPC_CREAT | 0777);
      extra->sharedInfo.shmaddr = shmat(extra->sharedInfo.shmid, NULL, 0);
      extra->frame->data = extra->sharedInfo.shmaddr;
      XShmAttach(extra->display, &extra->sharedInfo);
   } else {
      rsLog("X11 shared memory not supported");
      extra->frame = NULL;
   }

   clock_gettime(CLOCK_MONOTONIC, &extra->frameTime);
   system->destroy = xlibSystemDestroy;
   system->getFrame = xlibSystemGetFrame;
   system->wantsSave = xlibSystemWantsSave;
   return true;
}
