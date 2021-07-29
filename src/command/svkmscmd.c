/*
 * Copyright (C) 2021  Joshua Minter
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

#include "../device/device.h"
#include "../socket.h"
#include "command.h"
#include "rsbuild.h"
#include <libavutil/avutil.h>
#include <libavutil/hwcontext_drm.h>
#include <signal.h>
#ifdef RS_BUILD_POSIX_IO_FOUND
#include <sys/stat.h>
#endif

static volatile sig_atomic_t running = 1;

static void kmsSignal(int sig) {
   av_log(NULL, AV_LOG_INFO, "\nExiting...\n");
   running = 0;
   signal(sig, SIG_DFL);
}

static int kmsConnection(RSSocket *sock) {
   int ret;
   RSServiceDeviceInfo info;
   char *deviceName = NULL;
   RSDevice device = {0};
   AVFrame *frame = av_frame_alloc();
   if (frame == NULL) {
      ret = AVERROR(ENOMEM);
      goto error;
   }
   if ((ret = rsSocketReceive(sock, sizeof(info), &info, 0, NULL)) < 0) {
      goto error;
   }

   deviceName = av_mallocz((size_t)(info.deviceLength + 1));
   if (deviceName == NULL) {
      ret = AVERROR(ENOMEM);
      goto error;
   }
   if ((ret = rsSocketReceive(sock, info.deviceLength, deviceName, 0, NULL)) < 0) {
      goto error;
   }

   av_log(NULL, AV_LOG_INFO, "Framerate = %i, Device = %s\n", info.framerate, deviceName);
   if ((ret = rsKmsDeviceCreate(&device, deviceName, info.framerate)) < 0) {
      goto error;
   }

   AVHWFramesContext *hwFramesCtx = (AVHWFramesContext *)device.hwFrames->data;
   AVDRMDeviceContext *drmDeviceCtx = hwFramesCtx->device_ctx->hwctx;
   if ((ret = rsSocketSend(sock, sizeof(AVCodecParameters), device.params, 1,
                           &drmDeviceCtx->fd)) < 0) {
      goto error;
   }

   while (running) {
      int64_t pts;
      if ((ret = rsDeviceNextFrame(&device, frame)) < 0) {
         pts = ret;
      } else {
         pts = frame->pts;
      }
      if ((ret = rsSocketSend(sock, sizeof(int64_t), &pts, 0, NULL)) < 0) {
         goto error;
      }
      if (pts < 0) {
         continue;
      }

      int objects[AV_DRM_MAX_PLANES];
      AVDRMFrameDescriptor *desc = (AVDRMFrameDescriptor *)frame->data[0];
      for (int i = 0; i < desc->nb_objects; ++i) {
         objects[i] = desc->objects[i].fd;
      }
      if ((ret = rsSocketSend(sock, sizeof(AVDRMFrameDescriptor), desc,
                              (size_t)desc->nb_objects, objects)) < 0) {
         goto error;
      }
      av_frame_unref(frame);
   }

   ret = 0;
error:
   rsDeviceDestroy(&device);
   av_freep(&deviceName);
   av_frame_free(&frame);
   return ret;
}

int rsKmsService(void) {
   int ret;
   RSSocket sock = {0};
#ifdef RS_BUILD_POSIX_IO_FOUND
   umask(0000);
#else
   av_log(NULL, AV_LOG_WARNING, "Failed to change umask: Posix I/O was not found during compilation\n");
#endif
   if ((ret = rsSocketCreate(&sock)) < 0) {
      goto error;
   }
   if ((ret = rsSocketBind(&sock, RS_SERVICE_DEVICE_PATH)) < 0) {
      goto error;
   }

   signal(SIGINT, kmsSignal);
   signal(SIGTERM, kmsSignal);
   while (running) {
      RSSocket conn = {0};
      if ((ret = rsSocketAccept(&sock, &conn, -1)) < 0) {
         if (ret == AVERROR(EAGAIN)) {
            continue;
         } else {
            goto error;
         }
      }

      ret = kmsConnection(&conn);
      rsSocketDestroy(&conn);
      if (ret < 0) {
         av_log(NULL, AV_LOG_WARNING, "Disconnected: %s\n", av_err2str(ret));
      }
   }

   ret = 0;
error:
   rsSocketDestroy(&sock);
   return ret;
}
