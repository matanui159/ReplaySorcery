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

int rsX264EncoderCreate(RSEncoder *encoder, const AVFrame *frame) {
   int ret;
   AVFrame *clone = av_frame_clone(frame);
   if (clone == NULL) {
      goto error;
   }
   if ((ret = rsFFmpegEncoderCreate(encoder, "libx264")) < 0) {
      goto error;
   }

   rsFFmpegEncoderSetOption(encoder, "forced-idr", "true");
   if (rsConfig.videoQuality != RS_CONFIG_AUTO) {
      rsFFmpegEncoderSetOption(encoder, "qp", "%i", rsConfig.videoQuality);
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

   int scaleWidth = rsConfig.scaleWidth;
   if (scaleWidth == RS_CONFIG_AUTO) {
      scaleWidth = frame->width;
   }
   int scaleHeight = rsConfig.scaleHeight;
   if (scaleHeight == RS_CONFIG_AUTO) {
      scaleHeight = frame->height;
   }
   if ((ret = rsFFmpegEncoderOpen(encoder, "scale=%ix%i,format=yuv420p", scaleWidth,
                                  scaleHeight)) < 0) {
      goto error;
   }
   if ((ret = rsEncoderSendFrame(encoder, clone)) < 0) {
      goto error;
   }
   av_frame_free(&clone);

   return 0;
error:
   av_frame_free(&clone);
   rsEncoderDestroy(encoder);
   return ret;
}
