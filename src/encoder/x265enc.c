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

int rsX265EncoderCreate(RSEncoder *encoder, const AVCodecParameters *params) {
   int ret;
   int scaleWidth = params->width;
   int scaleHeight = params->height;
   rsScaleSize(&scaleWidth, &scaleHeight);
   if ((ret = rsFFmpegEncoderCreate(encoder, "libx265", "scale=%ix%i,format=yuv420p",
                                    scaleWidth, scaleHeight)) < 0) {
      goto error;
   }

   AVCodecContext *codecCtx = rsFFmpegEncoderGetContext(encoder);
   codecCtx->profile = FF_PROFILE_HEVC_MAIN;
   rsFFmpegEncoderSetOption(encoder, "forced-idr", "true");
   if (rsConfig.videoQuality != RS_CONFIG_AUTO) {
      if (rsConfig.videoPreset == RS_CONFIG_PRESET_FAST) {
         rsFFmpegEncoderSetOption(encoder, "x265-params", "qp=%i", rsConfig.videoQuality);
      } else {
         rsFFmpegEncoderSetOption(encoder, "crf", "%i", rsConfig.videoQuality);
      }
   }
   switch (rsConfig.videoPreset) {
   case RS_CONFIG_PRESET_FAST:
      rsFFmpegEncoderSetOption(encoder, "preset", "ultrafast");
      break;
   case RS_CONFIG_PRESET_MEDIUM:
      rsFFmpegEncoderSetOption(encoder, "preset", "medium");
      break;
   case RS_CONFIG_PRESET_SLOW:
      rsFFmpegEncoderSetOption(encoder, "preset", "slower");
      break;
   }
   if ((ret = rsFFmpegEncoderOpen(encoder, params, NULL)) < 0) {
      goto error;
   }

   return 0;
error:
   rsEncoderDestroy(encoder);
   return ret;
}
