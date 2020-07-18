/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "compress.h"
#include "config.h"
#include "std.h"
#include "system/xlib.h"
#include "util/circle.h"
#include "util/log.h"
#include <signal.h>

static sig_atomic_t mainRunning = true;

static void mainSignal(int signal) {
   const char *error = NULL;
   switch (signal) {
   case SIGSEGV:
      error = "Segment violation";
      break;
   case SIGILL:
      error = "Illegal program";
      break;
   case SIGFPE:
      error = "Floating point error";
      break;
   case SIGINT:
   case SIGTERM:
      mainRunning = false;
      break;
   }

   if (error != NULL) {
      rsError("Signal error: %s", error);
   }
}

int main(int argc, char *argv[]) {
   (void)argc;
   (void)argv;
   signal(SIGSEGV, mainSignal);
   signal(SIGILL, mainSignal);
   signal(SIGFPE, mainSignal);
   signal(SIGINT, mainSignal);
   signal(SIGTERM, mainSignal);

   RSConfig config;
   RSSystem system;
   RSCompress compress;
   RSBufferCircle circle;
   rsConfigDefaults(&config);
   rsXlibSystemCreate(&system, &config);
   rsCompressCreate(&compress, &config);
   size_t capacity = (size_t)(config.duration * config.framerate);
   rsBufferCircleCreate(&circle, capacity);

   while (mainRunning) {
      RSFrame frame;
      rsSystemFrameCreate(&frame, &system);
      rsCompress(&compress, rsBufferCircleNext(&circle), &frame);
      rsFrameDestroy(&frame);
      if (rsSystemWantsSave(&system)) {
         // TODO: save
      }
   }

   rsBufferCircleDestroy(&circle);
   rsCompressDestroy(&compress);
   rsSystemDestroy(&system);
}
