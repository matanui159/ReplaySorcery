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

#include "main.h"
#include "audio/audio.h"
#include "config.h"
#include "control/control.h"
#include "device/device.h"
#include "encoder/encoder.h"
#include "log.h"
#include "output.h"
#include "stream.h"
#include <libavutil/avutil.h>
#include <signal.h>

const char *rsName = "ReplaySorcery";
const char *rsLicense = "ReplaySorcery  Copyright (C) 2020  ReplaySorcery developers\n"
                        "This program comes with ABSOLUTELY NO WARRANTY.\n"
                        "This is free software, and you are welcome to redistribute it\n"
                        "under certain conditions; see COPYING for details.";

static volatile int running = 1;

static void mainSignal(int signal) {
   (void)signal;
   av_log(NULL, AV_LOG_INFO, "\nExiting...\n");
   running = 0;
}

int main(int argc, char *argv[]) {
   (void)argc;
   (void)argv;
   int ret;
   RSDevice *device = NULL;
   RSEncoder *encoder = NULL;
   RSStream *stream = NULL;
   RSControl *controller = NULL;
   RSAudioThread *audioThread = NULL;

   rsLogInit();
   if ((ret = rsConfigInit()) < 0) {
      goto error;
   }

   av_log(NULL, AV_LOG_INFO, "%s\n", rsLicense);
   av_log(NULL, AV_LOG_INFO, "FFmpeg version: %s\n", av_version_info());

   if ((ret = rsVideoDeviceCreate(&device)) < 0) {
      goto error;
   }
   if ((ret = rsVideoEncoderCreate(&encoder, device)) < 0) {
      goto error;
   }
   if ((ret = rsStreamCreate(&stream, encoder)) < 0) {
      goto error;
   }
   if ((ret = rsDefaultControlCreate(&controller)) < 0) {
      goto error;
   }
   if ((ret = rsAudioThreadCreate(&audioThread)) < 0) {
      av_log(NULL, AV_LOG_WARNING, "Failed to create audio thread: %s\n",
             av_err2str(ret));
   }

   signal(SIGINT, mainSignal);
   signal(SIGTERM, mainSignal);
   while (running) {
      if ((ret = rsStreamUpdate(stream)) < 0) {
         goto error;
      }
      if ((ret = rsControlWantsSave(controller)) < 0) {
         goto error;
      }
      if (ret > 0) {
         RSStream *audioStream = NULL;
         if (audioThread != NULL) {
            audioStream = audioThread->stream;
         }
         if ((ret = rsOutput(stream, audioStream)) < 0) {
            goto error;
         }
      }
   }

   ret = 0;
error:
   rsAudioThreadDestroy(&audioThread);
   rsControlDestroy(&controller);
   rsStreamDestroy(&stream);
   rsEncoderDestroy(&encoder);
   rsDeviceDestroy(&device);
   if (ret < 0) {
      av_log(NULL, AV_LOG_FATAL, "%s\n", av_err2str(ret));
      return EXIT_FAILURE;
   }
   return EXIT_SUCCESS;
}
