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

#include "../config.h"
#include "encoder.h"
#include "ffenc.h"

int rsVaapiEncoderCreate(RSEncoder *encoder, const AVCodecParameters *params,
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
   int scaleWidth = rsConfig.scaleWidth;
   if (scaleWidth == RS_CONFIG_AUTO) {
      scaleWidth = width;
   }
   int scaleHeight = rsConfig.scaleHeight;
   if (scaleHeight == RS_CONFIG_AUTO) {
      scaleHeight = height;
   }
   if ((ret = rsFFmpegEncoderCreate(
            encoder, "h264_vaapi",
            "hwmap=derive_device=vaapi,crop=%i:%i:%i:%i,scale_vaapi=%i:%i:nv12", width,
            height, rsConfig.videoX, rsConfig.videoY, scaleWidth, scaleHeight)) < 0) {
      goto error;
   }

   AVCodecContext *codecCtx = rsFFmpegEncoderGetContext(encoder);
   codecCtx->sw_pix_fmt = AV_PIX_FMT_NV12;
   if (rsConfig.videoQuality != RS_CONFIG_AUTO) {
      rsFFmpegEncoderSetOption(encoder, "qp", "%i", rsConfig.videoQuality);
   }
   switch (rsConfig.videoPreset) {
   case RS_CONFIG_PRESET_FAST:
      codecCtx->compression_level = 2;
      rsFFmpegEncoderSetOption(encoder, "coder", "cavlc");
      break;
   case RS_CONFIG_PRESET_MEDIUM:
      codecCtx->compression_level = 4;
      break;
   case RS_CONFIG_PRESET_SLOW:
      codecCtx->compression_level = 6;
      break;
   }
   if ((ret = rsFFmpegEncoderOpen(encoder, params, hwFrames)) < 0) {
      goto error;
   }
   // TODO: copy extradata from x264 if encoder does not support it

   return 0;
error:
   rsEncoderDestroy(encoder);
   return ret;
}
