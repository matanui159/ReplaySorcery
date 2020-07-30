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

#include "compress.h"
#include "config.h"
#include "output.h"
#include "std.h"
#include "system/xlib.h"
#include "util/circle.h"
#include "util/log.h"
#include "util/audio.h"
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

int want_to_save = 0;
typedef struct video_data {
	RSConfig* config;
	RSSystem* system;
	RSCompress* compress;
	RSBufferCircle* circle;
} video_data;

void* video_thread(void* data) {
	want_to_save = 0;
	video_data* vd = (video_data*)data;
	while (!rsSystemWantsSave(vd->system)) {
		RSFrame frame;
		rsSystemFrameCreate(&frame, vd->system);
		rsCompress(vd->compress, rsBufferCircleNext(vd->circle), &frame);
		rsFrameDestroy(&frame);

	}
	want_to_save = 1;
	return 0;
}


void* audio_thread(void* data) {
	while (!want_to_save) {
		RSAudio* audio = (RSAudio*)data;
		rsAudioGrabSample(audio);
		usleep(10000);
	}
	return 0;
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

   RSConfig config = {0};
   RSSystem system = {0};
   RSCompress compress = {0};
   RSBufferCircle circle = {0};
   RSOutput output = {0};
   rsConfigLoad(&config);
   // We have to call this one first since it might change the config width/height
   rsXlibSystemCreate(&system, &config);
   rsCompressCreate(&compress, &config);
   size_t capacity = (size_t)(config.duration * config.framerate);
   rsBufferCircleCreate(&circle, capacity);

	video_data dt = {
		.config = &config,
		.system = &system,
		.compress = &compress,
		.circle = &circle
	};
	pthread_t vthread;
	pthread_t athread;

   	while (mainRunning) {
		pthread_create(&vthread, NULL, &video_thread, &dt);
		pthread_create(&athread, NULL, audio_thread, &audio);
		
	//video_thread(&dt);
		pthread_join(vthread, NULL);
		pthread_join(athread, NULL);
		rsOutputDestroy(&output);
         	rsOutputCreate(&output, &config);
         	rsOutput(&output, &circle);

		usleep(999999999);
      	}

   rsOutputDestroy(&output);
   rsBufferCircleDestroy(&circle);
   rsCompressDestroy(&compress);
   rsSystemDestroy(&system);
   rsConfigDestroy(&config);
   rsAudioDestroy(&audio);
}
