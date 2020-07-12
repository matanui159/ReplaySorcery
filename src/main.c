/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "circle.h"
#include "common.h"
#include "compress.h"
#include "config/config.h"
#include "system/xlib.h"
#include <signal.h>
#include <time.h>

static bool mainRunning = true;

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
      rsError(RS_ERROR_SIGNAL_FRAME, "Signal error: %s", error);
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
   rsBufferCircleCreate(&circle, &config);

   while (mainRunning) {
      RSSystemFrame frame;
      rsSystemGetFrame(&system, &frame);
      rsCompress(&compress, rsBufferCircleAppend(&circle), &frame);
   }

   rsBufferCircleDestroy(&circle);
   rsCompressDestroy(&compress);
   rsSystemDestroy(&system);
}
