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

int rsNVEncoderCreate(RSEncoder *encoder, const AVCodecParameters *params,
                      const AVBufferRef *hwFrames) {
   int ret;
   int scaleWidth = params->width;
   int scaleHeight = params->height;
   rsScaleSize(&scaleWidth, &scaleHeight);
   if ((ret = rsFFmpegEncoderCreate(encoder, "h264_nvenc",
                                    "hwmap=derive_device=cuda,scale_cuda=%i:%i",
                                    scaleWidth, scaleHeight)) < 0) {
      goto error;
   }

   AVCodecContext *codecCtx = rsFFmpegEncoderGetContext(encoder);
   codecCtx->sw_pix_fmt = params->format;
   if (rsConfig.videoQuality != RS_CONFIG_AUTO) {
      rsFFmpegEncoderSetOption(encoder, "qp", "%i", rsConfig.videoQuality);
   }
   switch (rsConfig.videoPreset) {
   case RS_CONFIG_PRESET_FAST:
      rsFFmpegEncoderSetOption(encoder, "preset", "fast");
      break;
   case RS_CONFIG_PRESET_MEDIUM:
      rsFFmpegEncoderSetOption(encoder, "preset", "medium");
      break;
   case RS_CONFIG_PRESET_SLOW:
      rsFFmpegEncoderSetOption(encoder, "preset", "slow");
      break;
   }
   if ((ret = rsFFmpegEncoderOpen(encoder, params, hwFrames)) < 0) {
      goto error;
   }

   av_log(NULL, AV_LOG_WARNING, "NVENC backend is currently very experimental\n");
   return 0;
error:
   rsEncoderDestroy(encoder);
   return ret;
}
