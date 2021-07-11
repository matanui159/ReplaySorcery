/*
 * Copyright (C) 2020-2021  Joshua Minter
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

#include "audio/abuffer.h"
#include "audio/audio.h"
#include "buffer.h"
#include "command/command.h"
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
static AVPacket *videoPacket;
static AVFrame *videoFrame;
static RSAudioThread audioThread;
static RSControl controller;
static int silence = 0;
static volatile sig_atomic_t running = 1;

static void mainSignal(int sig) {
   av_log(NULL, AV_LOG_INFO, "\nExiting...\n");
   running = 0;
   signal(sig, SIG_DFL);
}

static int mainCommand(const char *name) {
   if (strcmp(name, "kms-devices") == 0) {
      return rsKmsDevices();
   } else if (strcmp(name, "kms-service") == 0) {
      return rsKmsService();
   } else if (strcmp(name, "save") == 0) {
      return rsControlSave();
   } else {
      av_log(NULL, AV_LOG_ERROR, "Unknown command: %s\n", name);
      return AVERROR(ENOSYS);
   }
}

static void mainUnsilence(void) {
   if (silence) {
      rsLogSilence(-1);
      silence = 0;
   }
}

static int mainStep(void) {
   int ret;
   while ((ret = rsEncoderNextPacket(&videoEncoder, videoPacket)) == AVERROR(EAGAIN)) {
      if ((ret = rsDeviceNextFrame(&videoDevice, videoFrame)) < 0) {
         av_log(NULL, AV_LOG_WARNING, "Failed to get frame from device: %s\n",
                av_err2str(ret));
         if (!silence) {
            rsLogSilence(1);
            silence = 1;
         }
         return 0;
      }

      mainUnsilence();
      if ((ret = rsEncoderSendFrame(&videoEncoder, videoFrame)) < 0) {
         return ret;
      }
   }
   if ((ret = rsBufferAddPacket(&videoBuffer, videoPacket)) < 0) {
      return ret;
   }
   return 0;
}

static int mainOutput(void) {
   int ret;
   RSOutput output = {0};
   rsAudioThreadLock(&audioThread);
   if ((ret = rsOutputCreate(&output)) < 0) {
      goto error;
   }

   const AVCodecParameters *audioParams;
   if ((ret = rsAudioBufferGetParams(&audioThread.buffer, &audioParams)) < 0) {
      goto error;
   }

   rsOutputAddStream(&output, videoEncoder.params);
   rsOutputAddStream(&output, audioParams);
   if ((ret = rsOutputOpen(&output)) < 0) {
      goto error;
   }

   int64_t startTime;
   if ((startTime = rsBufferGetStartTime(&videoBuffer)) < 0) {
      ret = (int)startTime;
      goto error;
   }
   if ((ret = rsAudioBufferWrite(&audioThread.buffer, &output, 1, startTime)) < 0) {
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
   rsAudioThreadUnlock(&audioThread);
   return ret;
}

static int mainOutputVideo(void) {
   int ret;
   RSOutput output = {0};
   if ((ret = rsOutputCreate(&output)) < 0) {
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
   int ret;
   if ((ret = rsLogInit()) < 0) {
      goto error;
   }
   if (argc >= 2) {
      ret = mainCommand(argv[1]);
      goto error;
   }
   if ((ret = rsConfigInit()) < 0) {
      goto error;
   }

   av_log(NULL, AV_LOG_INFO, "%s\n",
          RS_NAME "  Copyright (C) 2020-2021  ReplaySorcery developers\n"
                  "This program comes with ABSOLUTELY NO WARRANTY.\n"
                  "This is free software, and you are welcome to redistribute it\n"
                  "under certain conditions; see COPYING for details.");
   av_log(NULL, AV_LOG_INFO, "FFmpeg version: %s\n", av_version_info());

   if ((ret = rsVideoDeviceCreate(&videoDevice)) < 0) {
      goto error;
   }
   if ((ret = rsVideoEncoderCreate(&videoEncoder, videoDevice.params,
                                   videoDevice.hwFrames)) < 0) {
      goto error;
   }
   if ((ret = rsBufferCreate(&videoBuffer)) < 0) {
      goto error;
   }

   videoPacket = av_packet_alloc();
   videoFrame = av_frame_alloc();
   if (videoPacket == NULL || videoFrame == NULL) {
      ret = AVERROR(ENOMEM);
      goto error;
   }
   if ((ret = rsAudioThreadCreate(&audioThread)) < 0) {
      av_log(NULL, AV_LOG_WARNING, "Failed to create audio thread: %s\n",
             av_err2str(ret));
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
         if (audioThread.running) {
            ret = mainOutput();
         } else {
            ret = mainOutputVideo();
         }
         if (ret < 0) {
            av_log(NULL, AV_LOG_WARNING, "Failed to output video: %s\n", av_err2str(ret));
         }
      }
   }

   ret = 0;
error:
   mainUnsilence();
   rsControlDestroy(&controller);
   rsAudioThreadDestroy(&audioThread);
   av_frame_free(&videoFrame);
   av_packet_free(&videoPacket);
   rsBufferDestroy(&videoBuffer);
   rsEncoderDestroy(&videoEncoder);
   rsDeviceDestroy(&videoDevice);
   rsConfigExit();
   rsLogExit();
   if (ret < 0) {
      av_log(NULL, AV_LOG_FATAL, "%s\n", av_err2str(ret));
      return EXIT_FAILURE;
   }
   return EXIT_SUCCESS;
}
