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

static int vaapiEncoderCreate(RSEncoder **encoder, RSDevice *input, int preset) {
   int ret;
   if ((ret = rsFFmpegEncoderCreate(encoder, "h264_vaapi", input)) < 0) {
      goto error;
   }

   AVCodecContext *codecCtx = rsFFmpegEncoderGetContext(*encoder);
   codecCtx->pix_fmt = AV_PIX_FMT_VAAPI;
   codecCtx->sw_pix_fmt = AV_PIX_FMT_NV12;
   codecCtx->width = rsConfig.videoWidth;
   codecCtx->height = rsConfig.videoHeight;
   codecCtx->framerate = av_make_q(1, rsConfig.videoFramerate);
   codecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
   codecCtx->profile = rsConfig.videoProfile;
   codecCtx->gop_size = rsConfig.videoGOP;
   if (rsConfig.videoQuality != RS_CONFIG_AUTO) {
      rsFFmpegEncoderOption(*encoder, "qp", "%i", rsConfig.videoQuality);
   }
   switch (preset) {
   case RS_CONFIG_PRESET_FAST:
      rsFFmpegEncoderOption(*encoder, "low_power", "true");
      // fallthrough
   case RS_CONFIG_PRESET_MEDIUM:
      rsFFmpegEncoderOption(*encoder, "coder", "cavlc");
      break;
   }
   if ((ret = rsFFmpegEncoderOpen(
            *encoder, "hwmap=derive_device=vaapi,crop=%i:%i:%i:%i,scale_vaapi=%i:%i:nv12",
            rsConfig.videoWidth, rsConfig.videoHeight, rsConfig.videoX, rsConfig.videoY,
            rsConfig.videoWidth, rsConfig.videoHeight)) < 0) {
      goto error;
   }
   // TODO: copy extradata from x264 if encoder does not support it

   return 0;
error:
   rsEncoderDestroy(encoder);
   return ret;
}

int rsVaapiEncoderCreate(RSEncoder **encoder, RSDevice *input) {
   int ret;
   if ((ret = vaapiEncoderCreate(encoder, input, rsConfig.videoPreset)) < 0) {
      if (rsConfig.videoPreset == RS_CONFIG_PRESET_FAST) {
         av_log(NULL, AV_LOG_WARNING, "Low power mode not supported\n");
         return vaapiEncoderCreate(encoder, input, RS_CONFIG_PRESET_MEDIUM);
      }
      return ret;
   }
   return 0;
}
