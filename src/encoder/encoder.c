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
#include "x264.h"

int rsVideoEncoderCreate(RSEncoder *encoder, const RSDevice *input) {
   int ret;
   switch (rsConfig.videoEncoder) {
   case RS_CONFIG_VIDEO_X264:
      return rsX264EncoderCreate(encoder, input, 0);
   case RS_CONFIG_VIDEO_X264L:
      return rsX264EncoderCreate(encoder, input, 1);
   }

   if ((ret = rsX264EncoderCreate(encoder, input, 1)) >= 0) {
      return 0;
   }
   av_log(NULL, AV_LOG_WARNING, "Failed to create x264 encoder: %s\n", av_err2str(ret));

   return AVERROR(ENOSYS);
}
