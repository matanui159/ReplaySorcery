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

#include "audio.h"
#include "compress.h"
#include "config.h"
#include "output.h"
#include "std.h"
#include "system/xlib.h"
#include "util/circle.h"
#include "util/log.h"
#include <signal.h>

sig_atomic_t mainRunning = true;
sig_atomic_t wantToSave = false;

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
   rsLog("ReplaySorcery  Copyright (C) 2020  Joshua Minter");
   rsLog("This program comes with ABSOLUTELY NO WARRANTY.");
   rsLog("This is free software, and you are welcome to redistribute it");
   rsLog("under certain conditions; see COPYING for details.");

   pthread_t athread;
   
   RSConfig config;
   RSSystem system;
   RSCompress compress;
   RSBufferCircle circle;
   RSOutput output = {0};
   RSAudio audio;
   rsConfigLoad(&config);
   // We have to call this one first since it might change the config width/height
   rsXlibSystemCreate(&system, &config);
   rsCompressCreate(&compress, &config);
   size_t capacity = (size_t)(config.duration * config.framerate);
   rsBufferCircleCreate(&circle, capacity);
   rsAudioCreate(&audio, &config);

   for (;;) {
      pthread_create(&athread, NULL, &rsAudioThread, &audio);
      while (!rsSystemWantsSave(&system) && mainRunning) {
      	RSFrame frame = {0};
      	rsSystemFrameCreate(&frame, &system);
      	rsCompress(&compress, rsBufferCircleNext(&circle), &frame);
      	rsFrameDestroy(&frame);
      }
      wantToSave = true;
      pthread_join(athread, NULL);

      if (!mainRunning) {
      	break;
      }
      
      rsOutputDestroy(&output);
      rsOutputCreate(&output, &config, &audio);
      rsOutput(&output, &circle);
      wantToSave = false;
   }

   rsOutputDestroy(&output);
   rsBufferCircleDestroy(&circle);
   rsCompressDestroy(&compress);
   rsSystemDestroy(&system);
   rsConfigDestroy(&config);
   rsAudioDestroy(&audio);
}
