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

#include "../util.h"
#include "command.h"
#include <libavutil/avutil.h>
#include <libavutil/hwcontext.h>
#include <rsbuild.h>
#ifdef RS_BUILD_LIBDRM_FOUND
#include <libavutil/hwcontext_drm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#endif

#ifdef RS_BUILD_LIBDRM_FOUND
static int kmsCardDevices(int index) {
   int ret;
   AVBufferRef *deviceRef = NULL;
   drmModePlaneRes *planeList = NULL;
   char *path = rsFormat("/dev/dri/card%i", index);
   if (path == NULL) {
      ret = AVERROR(ENOMEM);
      goto error;
   }
   if ((ret = av_hwdevice_ctx_create(&deviceRef, AV_HWDEVICE_TYPE_DRM, path, NULL, 0)) <
       0) {
      if (ret != AVERROR(ENOENT)) {
         av_log(NULL, AV_LOG_ERROR, "Failed to create DRM device: %s\n", av_err2str(ret));
      }
      goto error;
   }

   AVHWDeviceContext *deviceCtx = (AVHWDeviceContext *)deviceRef->data;
   AVDRMDeviceContext *drmDevice = deviceCtx->hwctx;
   if ((ret = drmSetClientCap(drmDevice->fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1)) < 0) {
      av_log(NULL, AV_LOG_WARNING, "Failed to enable DRM universal planes\n");
   }

   planeList = drmModeGetPlaneResources(drmDevice->fd);
   if (planeList == NULL) {
      ret = AVERROR(errno);
      av_log(NULL, AV_LOG_ERROR, "Failed to get DRM plane resources: %s\n",
             av_err2str(ret));
      goto error;
   }
   for (uint32_t i = 0; i < planeList->count_planes; ++i) {
      uint32_t planeID = planeList->planes[i];
      drmModePlane *plane = drmModeGetPlane(drmDevice->fd, planeID);
      if (plane == NULL) {
         ret = AVERROR(errno);
         av_log(NULL, AV_LOG_ERROR, "Failed to get DRM plane %" PRIu32 ": %s\n", planeID,
                av_err2str(ret));
         goto error;
      }
      if (plane->fb_id == 0) {
         drmModeFreePlane(plane);
         continue;
      }

      drmModeFB *framebuffer = drmModeGetFB(drmDevice->fd, plane->fb_id);
      drmModeFreePlane(plane);
      if (framebuffer == NULL) {
         ret = AVERROR(errno);
         av_log(NULL, AV_LOG_ERROR, "Failed to get DRM framebuffer %" PRIu32 ": %s\n",
                plane->fb_id, av_err2str(ret));
         goto error;
      }

      av_log(NULL, AV_LOG_INFO,
             "card%i:%" PRIu32 " (%" PRIu32 "x%" PRIu32 "x%" PRIu32 ")\n", index, planeID,
             framebuffer->width, framebuffer->height, framebuffer->depth);
   }

   ret = 0;
error:
   if (planeList != NULL) {
      drmModeFreePlaneResources(planeList);
   }
   av_buffer_unref(&deviceRef);
   av_freep(&path);
   return ret;
}
#endif

int rsKmsDevices(void) {
#ifdef RS_BUILD_LIBDRM_FOUND
   int ret;
   for (int i = 0; (ret = kmsCardDevices(i)) >= 0; ++i) {
   }
   if (ret != AVERROR(ENOENT)) {
      return ret;
   }
   return 0;
#else

   av_log(NULL, AV_LOG_ERROR, "libdrm was not found during compilation\n");
   return AVERROR(ENOSYS);
#endif
}
