/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "common.h"
#include "compress.h"
#include "config/config.h"
#include "system/xlib.h"
#include <signal.h>
#include <time.h>

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

   RSConfig config;
   rsConfigDefaults(&config);

   RSSystem system;
   rsXlibSystemCreate(&system, &config);

   RSCompress compress;
   RSBuffer buffer;
   rsCompressCreate(&compress, &config);
   rsBufferCreate(&buffer, 1024);

   clock_t start = clock();
   int framerate = 0;
   while (clock() - start < CLOCKS_PER_SEC * 4) {
      RSSystemFrame frame;
      rsSystemGetFrame(&system, &frame);
      rsCompress(&compress, &buffer, &frame);

      if (clock() - start >= CLOCKS_PER_SEC) {
         ++framerate;
      }
   }
   rsLog("Memory size: %zu", buffer.size);
   rsLog("FPS: %i", framerate / 3);

   FILE *jpeg = fopen("test.jpg", "wb");
   fwrite(buffer.data, buffer.size, 1, jpeg);
   fclose(jpeg);

   rsBufferDestroy(&buffer);
   rsCompressDestroy(&compress);
   rsSystemDestroy(&system);
}
