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

int rsX264EncoderCreate(RSEncoder *encoder, RSDevice *input) {
   AVDictionary *options = NULL;
   av_dict_set_int(&options, "qp", rsConfig.videoQuality, 0);
   switch (rsConfig.videoPreset) {
   case RS_CONFIG_PRESET_FAST:
      av_dict_set(&options, "preset", "ultrafast", 0);
      break;
   case RS_CONFIG_PRESET_MEDIUM:
      av_dict_set(&options, "preset", "medium", 0);
      break;
   case RS_CONFIG_PRESET_SLOW:
      av_dict_set(&options, "preset", "slower", 0);
      break;
   }
   if (av_dict_count(options) != 2) {
      av_dict_free(&options);
      return AVERROR(ENOMEM);
   }

   int ret = rsEncoderCreate(encoder, &(RSEncoderParams){
                                          .name = "libx264",
                                          .options = options,
                                          .input = input,
                                          .swFormat = AV_PIX_FMT_YUV420P,
                                      });
   av_dict_free(&options);
   return ret;
}
