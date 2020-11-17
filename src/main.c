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
#include "buffer.h"
#include "config.h"
#include "control/control.h"
#include "device/device.h"
#include "encoder/encoder.h"
#include "log.h"
#include "output.h"
#include <libavutil/avutil.h>
#include <signal.h>

const char *rsName = "ReplaySorcery";
const char *rsLicense = "ReplaySorcery  Copyright (C) 2020  ReplaySorcery developers\n"
                        "This program comes with ABSOLUTELY NO WARRANTY.\n"
                        "This is free software, and you are welcome to redistribute it\n"
                        "under certain conditions; see COPYING for details.";

static RSDevice mainDevice;
static RSEncoder mainEncoder;
static RSBuffer mainBuffer;
static AVPacket mainPacket;
static AVFrame *mainFrame;
static volatile sig_atomic_t mainRunning = 1;

static void mainSignal(int signal) {
   (void)signal;
   av_log(NULL, AV_LOG_INFO, "\nExiting...\n");
   mainRunning = 0;
}

static int mainStep(void) {
   int ret;
   while ((ret = rsEncoderGetPacket(&mainEncoder, &mainPacket)) == AVERROR(EAGAIN)) {
      if ((ret = rsDeviceGetFrame(&mainDevice, mainFrame)) < 0) {
         return ret;
      }
      if ((ret = rsEncoderSendFrame(&mainEncoder, mainFrame)) < 0) {
         return ret;
      }
   }
   if (ret < 0) {
      return ret;
   }
   if ((ret = rsBufferAddPacket(&mainBuffer, &mainPacket)) < 0) {
      return ret;
   }
   return 0;
}

static void mainOutput(void) {
   int ret;
   RSOutput output = {0};
   int64_t startTime;
   if ((startTime = rsBufferStartTime(&mainBuffer)) < 0) {
      ret = (int)startTime;
      goto error;
   }
   if ((ret = rsOutputCreate(&output, startTime)) < 0) {
      goto error;
   }

   rsOutputAddStream(&output, mainEncoder.params);
   if ((ret = rsOutputOpen(&output)) < 0) {
      goto error;
   }
   if ((ret = rsBufferWrite(&mainBuffer, &output, 0)) < 0) {
      goto error;
   }
   if ((ret = rsOutputClose(&output)) < 0) {
      goto error;
   }
   rsOutputDestroy(&output);

   return;
error:
   rsOutputDestroy(&output);
   av_log(NULL, AV_LOG_WARNING, "Failed to output video: %s\n", av_err2str(ret));
}

int main(int argc, char *argv[]) {
   (void)argc;
   (void)argv;
   int ret;
   RSControl *controller = NULL;
   RSAudioThread *audioThread = NULL;

   rsLogInit();
   if ((ret = rsConfigInit()) < 0) {
      goto error;
   }

   av_log(NULL, AV_LOG_INFO, "%s\n", rsLicense);
   av_log(NULL, AV_LOG_INFO, "FFmpeg version: %s\n", av_version_info());

   if ((ret = rsVideoDeviceCreate(&mainDevice)) < 0) {
      goto error;
   }
   if ((ret = rsVideoEncoderCreate(&mainEncoder, mainDevice.params)) < 0) {
      goto error;
   }
   if ((ret = rsBufferCreate(&mainBuffer)) < 0) {
      goto error;
   }

   av_init_packet(&mainPacket);
   mainFrame = av_frame_alloc();
   if (mainFrame == NULL) {
      ret = AVERROR(ENOMEM);
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
   while (mainRunning) {
      if ((ret = mainStep()) < 0) {
         goto error;
      }
      if ((ret = rsControlWantsSave(controller)) < 0) {
         goto error;
      }
      if (ret > 0) {
         mainOutput();
      }
   }

   ret = 0;
error:
   rsAudioThreadDestroy(&audioThread);
   rsControlDestroy(&controller);
   rsBufferDestroy(&mainBuffer);
   rsEncoderDestroy(&mainEncoder);
   rsDeviceDestroy(&mainDevice);
   if (ret < 0) {
      av_log(NULL, AV_LOG_FATAL, "%s\n", av_err2str(ret));
      return EXIT_FAILURE;
   }
   return EXIT_SUCCESS;
}
