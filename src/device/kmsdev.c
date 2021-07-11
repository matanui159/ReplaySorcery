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

#include "device.h"
#include "ffdev.h"

int rsKmsDeviceCreate(RSDevice *device, const char *deviceName, int framerate) {
   int ret;
   AVFrame *frame = av_frame_alloc();
   if (frame == NULL) {
      return AVERROR(ENOMEM);
   }
   if ((ret = rsFFmpegDeviceCreate(device, "kmsgrab")) < 0) {
      goto error;
   }

   rsFFmpegDeviceSetOption(device, "framerate", "%i", framerate);
   if (strcmp(deviceName, "auto") != 0) {
      int cardID;
      int planeID;
      char c;
      if (sscanf(deviceName, "card%i:%i%c", &cardID, &planeID, &c) != 2) {
         av_log(NULL, AV_LOG_ERROR, "KMS device format: cardX:<plane ID>\n");
         ret = AVERROR(EINVAL);
         goto error;
      }
      rsFFmpegDeviceSetOption(device, "device", "/dev/dri/card%i", cardID);
      rsFFmpegDeviceSetOption(device, "plane_id", "%i", planeID);
   }
   if ((ret = rsFFmpegDeviceOpen(device, NULL)) < 0) {
      goto error;
   }
   if ((ret = rsDeviceNextFrame(device, frame)) < 0) {
      goto error;
   }

   device->hwFrames = av_buffer_ref(frame->hw_frames_ctx);
   if (device->hwFrames == NULL) {
      ret = AVERROR(ENOMEM);
      goto error;
   }
   av_frame_free(&frame);

   return 0;
error:
   av_frame_free(&frame);
   rsDeviceDestroy(device);
   return ret;
}
