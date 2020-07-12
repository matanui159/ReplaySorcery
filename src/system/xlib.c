#include "xlib.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include <sys/ipc.h>
#include <sys/shm.h>

typedef struct XlibSystemExtra {
   RSConfig config;
   Display *display;
   Window rootWindow;
   XImage *frame;
   XShmSegmentInfo sharedInfo;
   bool shared;
} XlibSystemExtra;

static int xlibError(Display *display, XErrorEvent *event) {
   char error[1024];
   XGetErrorText(display, event->error_code, error, sizeof(error));
   rsError(0, "X11 error: %s", error);
}

static void destroyXlibSystem(RSSystem *system) {
   XlibSystemExtra *extra = system->extra;
   if (extra->frame != NULL) {
      XDestroyImage(extra->frame);
   }
   XCloseDisplay(extra->display);
   rsFree(extra);
}

static void getXlibSystemFrame(RSSystem *system, RSSystemFrame *frame) {
   XlibSystemExtra *extra = system->extra;
   if (extra->shared) {
      XShmGetImage(extra->display, extra->rootWindow, extra->frame,
                   (int)extra->config.inputX, (int)extra->config.inputY, AllPlanes);
   } else {
      if (extra->frame != NULL) {
         XDestroyImage(extra->frame);
      }
      extra->frame = XGetImage(extra->display, extra->rootWindow, extra->config.inputX,
                               extra->config.inputY, (unsigned)extra->config.inputWidth,
                               (unsigned)extra->config.inputHeight, AllPlanes, ZPixmap);
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

bool rsCreateXlibSystem(RSSystem *system, const RSConfig *config) {
   system->extra = rsAllocate(sizeof(XlibSystemExtra));
   XlibSystemExtra *extra = system->extra;
   extra->config = *config;
   extra->display = XOpenDisplay(NULL);
   if (extra->display == NULL) {
      rsLog("Failed to open X11 display");
      return false;
   }
   XSetErrorHandler(xlibError);
   rsLog("X11 vendor: %s %i.%i.%i", ServerVendor(extra->display),
         ProtocolVersion(extra->display), ProtocolRevision(extra->display),
         VendorRelease(extra->display));
   extra->rootWindow = DefaultRootWindow(extra->display);

   int major, minor, pixmaps;
   extra->shared = XShmQueryVersion(extra->display, &major, &minor, &pixmaps);
   if (!pixmaps) {
      extra->shared = false;
   }
   if (extra->shared) {
      rsLog("X11 shared memory version: %i.%i", major, minor);
      rsClear(&extra->sharedInfo, sizeof(XShmSegmentInfo));
      int screen = DefaultScreen(extra->display);
      extra->frame =
          XShmCreateImage(extra->display, DefaultVisual(extra->display, screen),
                          (unsigned)DefaultDepth(extra->display, screen), ZPixmap, NULL,
                          &extra->sharedInfo, (unsigned)config->inputWidth,
                          (unsigned)config->inputHeight);
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

   system->destroy = destroyXlibSystem;
   system->getFrame = getXlibSystemFrame;
   return true;
}
