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

#include "aencoder.h"
#include "../config.h"

int rsAudioEncoderCreate(RSEncoder *encoder, const AVCodecParameters *params) {
   int ret;
   (void)encoder;
   (void)params;
   switch (rsConfig.audioEncoder) {
   case RS_CONFIG_ENCODER_AAC:
      return rsAacEncoderCreate(encoder, params);
   case RS_CONFIG_ENCODER_FDK:
      return rsAacEncoderCreate(encoder, params);
   }

   if ((ret = rsFdkEncoderCreate(encoder, params)) >= 0) {
      av_log(NULL, AV_LOG_INFO, "Created FDK-AAC audio encoder\n");
      return 0;
   }
   av_log(NULL, AV_LOG_WARNING, "Failed to create FDK-AAC audio encoder: %s\n",
          av_err2str(ret));

   if ((ret = rsAacEncoderCreate(encoder, params)) >= 0) {
      av_log(NULL, AV_LOG_INFO, "Created AAC audio encoder\n");
      return 0;
   }
   av_log(NULL, AV_LOG_WARNING, "Failed to create AAC audio encoder: %s\n",
          av_err2str(ret));

   return AVERROR(ENOSYS);
}
