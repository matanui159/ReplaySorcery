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

#include "../config.h"
#include "../util.h"
#include "encoder.h"
#include "ffenc.h"

int rsVaapiHevcEncoderCreate(RSEncoder *encoder, const AVCodecParameters *params,
                             const AVBufferRef *hwFrames) {
   int ret;
   int width = rsConfig.videoWidth;
   if (width == RS_CONFIG_AUTO) {
      width = params->width - rsConfig.videoX;
   }
   int height = rsConfig.videoHeight;
   if (height == RS_CONFIG_AUTO) {
      height = params->height - rsConfig.videoY;
   }
   int scaleWidth = width;
   int scaleHeight = height;
   rsScaleSize(&scaleWidth, &scaleHeight);
   if ((ret = rsFFmpegEncoderCreate(
            encoder, "hevc_vaapi",
            "hwmap=derive_device=vaapi,crop=%i:%i:%i:%i,scale_vaapi=%i:%i:nv12", width,
            height, rsConfig.videoX, rsConfig.videoY, scaleWidth, scaleHeight)) < 0) {
      goto error;
   }

   AVCodecContext *codecCtx = rsFFmpegEncoderGetContext(encoder);
   codecCtx->sw_pix_fmt = AV_PIX_FMT_NV12;
   codecCtx->profile = FF_PROFILE_UNKNOWN;
   if (rsConfig.videoQuality != RS_CONFIG_AUTO) {
      codecCtx->global_quality = rsConfig.videoQuality;
      if (rsConfig.videoBitrate != RS_CONFIG_AUTO) {
         rsFFmpegEncoderSetOption(encoder, "rc_mode", "QVBR");
      } else if (rsConfig.videoPreset == RS_CONFIG_PRESET_FAST) {
         rsFFmpegEncoderSetOption(encoder, "rc_mode", "CQP");
      }
   }
   switch (rsConfig.videoPreset) {
   case RS_CONFIG_PRESET_FAST:
      codecCtx->compression_level = 6;
      break;
   case RS_CONFIG_PRESET_MEDIUM:
      codecCtx->compression_level = 4;
      break;
   case RS_CONFIG_PRESET_SLOW:
      codecCtx->compression_level = 2;
      break;
   }
   if ((ret = rsFFmpegEncoderOpen(encoder, params, hwFrames)) < 0) {
      goto error;
   }

   return 0;
error:
   rsEncoderDestroy(encoder);
   return ret;
}
