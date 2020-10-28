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

int rsX264EncoderCreate(RSEncoder **encoder, RSDevice *input) {
   int ret;
   if ((ret = rsFFmpegEncoderCreate(encoder, "libx264", input)) < 0) {
      goto error;
   }

   AVCodecContext *codecCtx = rsFFmpegEncoderGetContext(*encoder);
   codecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
   codecCtx->thread_count = 1;
   codecCtx->gop_size = 0;
   codecCtx->profile = rsConfig.videoProfile;
   rsFFmpegEncoderOption(*encoder, "qp", "%i", rsConfig.videoQuality);
   switch (rsConfig.videoPreset) {
   case RS_CONFIG_PRESET_FAST:
      rsFFmpegEncoderOption(*encoder, "preset", "ultrafast");
      break;
   case RS_CONFIG_PRESET_MEDIUM:
      rsFFmpegEncoderOption(*encoder, "preset", "medium");
      break;
   case RS_CONFIG_PRESET_SLOW:
      rsFFmpegEncoderOption(*encoder, "preset", "slower");
      break;
   }
   if ((ret = rsFFmpegEncoderOpen(*encoder, "format=yuv420p")) < 0) {
      goto error;
   }

   return 0;
error:
   rsEncoderDestroy(encoder);
   return ret;
}
