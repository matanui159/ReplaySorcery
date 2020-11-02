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
#include "../util.h"
#include "encoder.h"
#include "ffenc.h"

int rsVaapiEncoderCreate(RSEncoder **encoder, RSDevice *input) {
   int ret;
   if ((ret = rsFFmpegEncoderCreate(encoder, "h264_vaapi", input)) < 0) {
      goto error;
   }

   int width = rsConfig.videoWidth;
   if (width == RS_CONFIG_AUTO) {
      width = input->params->width - rsConfig.videoX;
   }
   int height = rsConfig.videoHeight;
   if (height == RS_CONFIG_AUTO) {
      height = input->params->height - rsConfig.videoY;
   }

   AVCodecContext *codecCtx = rsFFmpegEncoderGetContext(*encoder);
   codecCtx->pix_fmt = AV_PIX_FMT_VAAPI;
   codecCtx->sw_pix_fmt = AV_PIX_FMT_NV12;
   codecCtx->framerate = av_make_q(1, rsConfig.videoFramerate);
   codecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
   codecCtx->profile = rsConfig.videoProfile;
   codecCtx->gop_size = rsConfig.videoGOP;
   if (rsConfig.scaleWidth == RS_CONFIG_AUTO) {
      codecCtx->width = width;
   } else {
      codecCtx->width = rsConfig.scaleWidth;
   }
   if (rsConfig.scaleHeight == RS_CONFIG_AUTO) {
      codecCtx->height = height;
   } else {
      codecCtx->height = rsConfig.scaleHeight;
   }
   if (rsConfig.videoQuality != RS_CONFIG_AUTO) {
      rsFFmpegEncoderOption(*encoder, "qp", "%i", rsConfig.videoQuality);
   }
   switch (rsConfig.videoPreset) {
   case RS_CONFIG_PRESET_FAST:
      codecCtx->compression_level = 2;
      rsFFmpegEncoderOption(*encoder, "coder", "cavlc");
      break;
   case RS_CONFIG_PRESET_MEDIUM:
      codecCtx->compression_level = 4;
      break;
   case RS_CONFIG_PRESET_SLOW:
      codecCtx->compression_level = 6;
      break;
   }
   if ((ret = rsFFmpegEncoderOpen(
            *encoder, "hwmap=derive_device=vaapi,crop=%i:%i:%i:%i,scale_vaapi=%i:%i:nv12",
            width, height, rsConfig.videoX, rsConfig.videoY, codecCtx->width,
            codecCtx->height)) < 0) {
      goto error;
   }
   // TODO: copy extradata from x264 if encoder does not support it

   return 0;
error:
   rsEncoderDestroy(encoder);
   return ret;
}
