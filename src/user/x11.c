#include "x11.h"
#include "../config/config.h"
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <libavutil/avutil.h>
#include <time.h>

/**
 * Extra data for the X11 user object.
 */
typedef struct X11User {
   /**
    * The opened X11 display.
    */
   Display* display;

   /**
    * The root window of the X11 display.
    */
   Window root;
} X11User;

/**
 * The implementation of `user.destroy`.
 */
static void x11UserDestroy(RSUser* user) {
   X11User* extra = user->extra;
   XCloseDisplay(extra->display);
   av_freep(&user->extra);
}

/**
 * The implementation of `user.wait`.
 */
static void x11UserWait(RSUser* user) {
   X11User* extra = user->extra;
   XEvent event;
   for (;;) {
      if (XEventsQueued(extra->display, QueuedAfterFlush) > 0) {
         XNextEvent(extra->display, &event);
         if (event.type == KeyPress) break;
      } else {
         // A fast loop seems to block `signal` handlers so we sleep for 10 milliseconds.
         struct timespec sleep = {0, 10000000};
         nanosleep(&sleep, &sleep);
      }
   }
}

/**
 * Small utility to grab a key with mods.
 */
static void x11UserGrab(RSUser* user, int key, unsigned mods) {
   X11User* extra = user->extra;
   XGrabKey(extra->display, key, mods, extra->root, False, GrabModeAsync, GrabModeAsync);
}

int rsX11UserCreate(RSUser* user) {
   X11User* extra = av_malloc(sizeof(X11User));
   user->extra = extra;
   extra->display = XOpenDisplay(rsConfig.inputDisplay);
   if (extra == NULL) {
      av_log(NULL, AV_LOG_ERROR, "Failed to open X11 display\n");
      return AVERROR_EXTERNAL;
   }
   extra->root = DefaultRootWindow(extra->display);
   XSelectInput(extra->display, extra->root, KeyPressMask);

   int key = XKeysymToKeycode(extra->display, XK_R);
   // Mod4 is the super key.
   unsigned mods = ControlMask | Mod4Mask;
   x11UserGrab(user, key, mods);
   // Also capture when capslock or numlock (Mod2) is enabled.
   x11UserGrab(user, key, mods | LockMask);
   x11UserGrab(user, key, mods | Mod2Mask);
   x11UserGrab(user, key, mods | LockMask | Mod2Mask);

   user->destroy = x11UserDestroy;
   user->wait = x11UserWait;
   return 0;
}
