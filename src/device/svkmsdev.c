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

#include "../config.h"
#include "../socket.h"
#include "device.h"
#include "rsbuild.h"
#include <libavutil/avutil.h>
#include <libavutil/hwcontext.h>
#include <libavutil/hwcontext_drm.h>
#ifdef RS_BUILD_POSIX_IO_FOUND
#include <unistd.h>
#endif

static void kmsServiceBufferDestroy(void *extra, uint8_t *data) {
   (void)extra;
   AVDRMFrameDescriptor *desc = (AVDRMFrameDescriptor *)data;
#ifdef RS_BUILD_POSIX_IO_FOUND
   for (int i = 0; i < desc->nb_objects; ++i) {
      close(desc->objects[i].fd);
   }
#endif
   av_freep(&desc);
}

static void kmsServiceDeviceDestroy(RSDevice *device) {
   RSSocket *sock = device->extra;
   if (sock != NULL) {
      rsSocketDestroy(sock);
      av_freep(&sock);
   }
}

static int kmsServiceDeviceNextFrame(RSDevice *device, AVFrame *frame) {
   int ret;
   RSSocket *sock = device->extra;
   int objects[AV_DRM_MAX_PLANES];
   AVDRMFrameDescriptor *desc = av_mallocz(sizeof(AVDRMFrameDescriptor));
   if (desc == NULL) {
      ret = AVERROR(ENOMEM);
      goto error;
   }
   if ((ret = rsSocketReceive(sock, sizeof(int64_t), &frame->pts, 0, NULL)) < 0) {
      goto error;
   }
   if (frame->pts < 0) {
      ret = (int)frame->pts;
      av_log(NULL, AV_LOG_ERROR, "KMS service failed to get frame: %s\n",
             av_err2str(ret));
      goto error;
   }
   if ((ret = rsSocketReceive(sock, sizeof(AVDRMFrameDescriptor), desc, AV_DRM_MAX_PLANES,
                              objects)) < 0) {
      goto error;
   }
   for (int i = 0; i < desc->nb_objects; ++i) {
      desc->objects[i].fd = objects[i];
   }

   frame->hw_frames_ctx = av_buffer_ref(device->hwFrames);
   if (frame->hw_frames_ctx == NULL) {
      goto error;
   }

   frame->buf[0] = av_buffer_create((uint8_t *)desc, sizeof(AVDRMFrameDescriptor),
                                    kmsServiceBufferDestroy, NULL, 0);
   if (frame->buf[0] == NULL) {
      goto error;
   }
   frame->data[0] = (uint8_t *)desc;
   frame->width = device->params->width;
   frame->height = device->params->height;
   frame->format = AV_PIX_FMT_DRM_PRIME;

   return 0;
error:
   av_freep(&desc);
   av_frame_unref(frame);
   return ret;
}

int rsKmsServiceDeviceCreate(RSDevice *device) {
   int ret;
   AVBufferRef *hwDeviceRef = NULL;
   if ((ret = rsDeviceCreate(device)) < 0) {
      goto error;
   }

   RSSocket *sock = av_mallocz(sizeof(RSSocket));
   hwDeviceRef = av_hwdevice_ctx_alloc(AV_HWDEVICE_TYPE_DRM);
   device->extra = sock;
   device->destroy = kmsServiceDeviceDestroy;
   device->nextFrame = kmsServiceDeviceNextFrame;
   device->hwFrames = av_hwframe_ctx_alloc(hwDeviceRef);
   if (sock == NULL || hwDeviceRef == NULL || device->hwFrames == NULL) {
      ret = AVERROR(ENOMEM);
      goto error;
   }
   if ((ret = rsSocketCreate(sock)) < 0) {
      goto error;
   }
   if ((ret = rsSocketConnect(sock, RS_SERVICE_DEVICE_PATH)) < 0) {
      goto error;
   }

   uint8_t deviceLength = (uint8_t)strlen(rsConfig.videoDevice);
   RSServiceDeviceInfo info = {.framerate = rsConfig.videoFramerate,
                               .deviceLength = deviceLength};
   if ((ret = rsSocketSend(sock, sizeof(RSServiceDeviceInfo), &info, 0, NULL)) < 0) {
      goto error;
   }
   if ((ret = rsSocketSend(sock, deviceLength, rsConfig.videoDevice, 0, NULL)) < 0) {
      goto error;
   }

   // this is super hacky lmao
   AVHWDeviceContext *hwDeviceCtx = (AVHWDeviceContext *)hwDeviceRef->data;
   AVDRMDeviceContext *drmDeviceCtx = hwDeviceCtx->hwctx;
   if ((ret = rsSocketReceive(sock, sizeof(AVCodecParameters), device->params, 1,
                              &drmDeviceCtx->fd)) < 0) {
      goto error;
   }
   av_buffer_unref(&hwDeviceRef);
   device->params->extradata = NULL;
   device->params->extradata_size = 0;

   AVHWFramesContext *hwFramesCtx = (AVHWFramesContext *)device->hwFrames->data;
   hwFramesCtx->width = device->params->width;
   hwFramesCtx->height = device->params->height;
   hwFramesCtx->format = AV_PIX_FMT_DRM_PRIME;
   hwFramesCtx->sw_format = device->params->format;
   if ((ret = av_hwframe_ctx_init(device->hwFrames)) < 0) {
      av_log(NULL, AV_LOG_ERROR, "Failed to init DRM frames context: %s\n",
             av_err2str(ret));
      goto error;
   }

   return 0;
error:
   av_buffer_unref(&hwDeviceRef);
   rsDeviceDestroy(device);
   return ret;
}
