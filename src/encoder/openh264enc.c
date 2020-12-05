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

int rsOpenH264EncoderCreate(RSEncoder *encoder, const AVCodecParameters *params) {
   int ret;
   int scaleWidth = rsConfig.scaleWidth;
   if (scaleWidth == RS_CONFIG_AUTO) {
      scaleWidth = params->width;
   }
   int scaleHeight = rsConfig.scaleHeight;
   if (scaleHeight == RS_CONFIG_AUTO) {
      scaleHeight = params->height;
   }
   if ((ret = rsFFmpegEncoderCreate(encoder, "libopenh264", "scale=%ix%i,format=yuv420p",
                                    scaleWidth, scaleHeight)) < 0) {
      goto error;
   }

   AVCodecContext *codecCtx = rsFFmpegEncoderGetContext(encoder);
   codecCtx->slices = 1;
   if (rsConfig.videoProfile == FF_PROFILE_H264_BASELINE) {
      av_log(NULL, AV_LOG_WARNING, "Baseline profile is not supported\n");
      rsFFmpegEncoderSetOption(encoder, "profile", "constrained_baseline");
   }
   if (rsConfig.videoQuality != RS_CONFIG_AUTO) {
      codecCtx->qmax = rsConfig.videoQuality;
      codecCtx->qmin = rsConfig.videoQuality / 2;
   }
   if (rsConfig.videoBitrate != RS_CONFIG_AUTO) {
      rsFFmpegEncoderSetOption(encoder, "allow_skip_frames", "1");
   }
   switch (rsConfig.videoPreset) {
   case RS_CONFIG_PRESET_FAST:
      rsFFmpegEncoderSetOption(encoder, "loopfilter", "0");
      rsFFmpegEncoderSetOption(encoder, "coder", "cavlc");
      break;
   case RS_CONFIG_PRESET_MEDIUM:
      rsFFmpegEncoderSetOption(encoder, "coder", "cavlc");
      break;
   case RS_CONFIG_PRESET_SLOW:
      rsFFmpegEncoderSetOption(encoder, "coder", "cabac");
   }
   if ((ret = rsFFmpegEncoderOpen(encoder, params, NULL)) < 0) {
      goto error;
   }

   return 0;
error:
   rsEncoderDestroy(encoder);
   return ret;
}
