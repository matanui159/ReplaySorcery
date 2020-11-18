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

#include "buffer.h"
#include "config.h"
#include "control/control.h"
#include "device/device.h"
#include "encoder/encoder.h"
#include "log.h"
#include "output.h"
#include "util.h"
#include <libavutil/avutil.h>
#include <signal.h>

static RSDevice videoDevice;
static RSEncoder videoEncoder;
static RSBuffer videoBuffer;
static AVPacket videoPacket;
static AVFrame *videoFrame;
static RSControl controller;
static volatile sig_atomic_t running = 1;

static void mainSignal(int signal) {
   (void)signal;
   av_log(NULL, AV_LOG_INFO, "\nExiting...\n");
   running = 0;
}

static int mainStep(void) {
   int ret;
   while ((ret = rsEncoderNextPacket(&videoEncoder, &videoPacket)) == AVERROR(EAGAIN)) {
      if ((ret = rsDeviceNextFrame(&videoDevice, videoFrame)) < 0) {
         return ret;
      }
      if ((ret = rsEncoderSendFrame(&videoEncoder, videoFrame)) < 0) {
         return ret;
      }
   }
   if (ret < 0) {
      return ret;
   }
   if ((ret = rsBufferAddPacket(&videoBuffer, &videoPacket)) < 0) {
      return ret;
   }
   return 0;
}

static int mainOutput(void) {
   int ret;
   RSOutput output = {0};
   int64_t startTime;
   if ((startTime = rsBufferStartTime(&videoBuffer)) < 0) {
      ret = (int)startTime;
      goto error;
   }
   if ((ret = rsOutputCreate(&output, startTime)) < 0) {
      goto error;
   }

   rsOutputAddStream(&output, videoEncoder.params);
   if ((ret = rsOutputOpen(&output)) < 0) {
      goto error;
   }
   if ((ret = rsBufferWrite(&videoBuffer, &output, 0)) < 0) {
      goto error;
   }
   if ((ret = rsOutputClose(&output)) < 0) {
      goto error;
   }

   ret = 0;
error:
   rsOutputDestroy(&output);
   return ret;
}

int main(int argc, char *argv[]) {
   (void)argc;
   (void)argv;
   int ret;
   rsLogInit();
   if ((ret = rsConfigInit()) < 0) {
      goto error;
   }

   av_log(NULL, AV_LOG_INFO, "%s\n",
          RS_NAME "  Copyright (C) 2020  ReplaySorcery developers\n"
                  "This program comes with ABSOLUTELY NO WARRANTY.\n"
                  "This is free software, and you are welcome to redistribute it\n"
                  "under certain conditions; see COPYING for details.");
   av_log(NULL, AV_LOG_INFO, "FFmpeg version: %s\n", av_version_info());

   av_init_packet(&videoPacket);
   videoFrame = av_frame_alloc();
   if (videoFrame == NULL) {
      ret = AVERROR(ENOMEM);
      goto error;
   }
   if ((ret = rsVideoDeviceCreate(&videoDevice)) < 0) {
      goto error;
   }
   if ((ret = rsDeviceNextFrame(&videoDevice, videoFrame)) < 0) {
      goto error;
   }
   if ((ret = rsVideoEncoderCreate(&videoEncoder, videoFrame)) < 0) {
      goto error;
   }
   av_frame_unref(videoFrame);

   if ((ret = rsBufferCreate(&videoBuffer)) < 0) {
      goto error;
   }
   if ((ret = rsDefaultControlCreate(&controller)) < 0) {
      goto error;
   }

   signal(SIGINT, mainSignal);
   signal(SIGTERM, mainSignal);
   while (running) {
      if ((ret = mainStep()) < 0) {
         goto error;
      }
      if ((ret = rsControlWantsSave(&controller)) < 0) {
         goto error;
      }
      if (ret > 0) {
         if ((ret = mainOutput()) < 0) {
            av_log(NULL, AV_LOG_WARNING, "Failed to output video: %s\n", av_err2str(ret));
         }
      }
   }

   ret = 0;
error:
   rsControlDestroy(&controller);
   av_frame_free(&videoFrame);
   rsBufferDestroy(&videoBuffer);
   rsEncoderDestroy(&videoEncoder);
   rsDeviceDestroy(&videoDevice);
   if (ret < 0) {
      av_log(NULL, AV_LOG_FATAL, "%s\n", av_err2str(ret));
      return EXIT_FAILURE;
   }
   return EXIT_SUCCESS;
}
