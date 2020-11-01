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

#include "encoder.h"
#include "../config.h"

void rsEncoderDestroy(RSEncoder **encoder) {
   if (*encoder != NULL && (*encoder)->destroy != NULL) {
      (*encoder)->destroy(*encoder);
   }
   av_freep(encoder);
}

int rsVideoEncoderCreate(RSEncoder **encoder, RSDevice *input) {
   int ret;
   switch (rsConfig.videoEncoder) {
   case RS_CONFIG_ENCODER_X264:
      return rsX264EncoderCreate(encoder, input);
   case RS_CONFIG_ENCODER_VAAPI:
      return rsVaapiEncoderCreate(encoder, input);
   }

   if (rsConfig.videoInput == RS_CONFIG_DEVICE_KMS) {
      if ((ret = rsVaapiEncoderCreate(encoder, input)) >= 0) {
         av_log(NULL, AV_LOG_INFO, "Created VAAPI encoder\n");
         return 0;
      }
      av_log(NULL, AV_LOG_WARNING, "Failed to create VAAPI encoder: %s\n",
             av_err2str(ret));

      return AVERROR(ENOSYS);
   }

   if ((ret = rsX264EncoderCreate(encoder, input)) >= 0) {
      av_log(NULL, AV_LOG_INFO, "Created x264 encoder\n");
      return 0;
   }
   av_log(NULL, AV_LOG_WARNING, "Failed to create x264 encoder: %s\n", av_err2str(ret));

   return AVERROR(ENOSYS);
}
