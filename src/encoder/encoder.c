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
#include "../util.h"
#include <libavutil/avutil.h>
#include <libavutil/pixdesc.h>

int rsEncoderCreate(RSEncoder *encoder) {
   rsClear(encoder, sizeof(RSEncoder));
   encoder->params = avcodec_parameters_alloc();
   if (encoder->params == NULL) {
      return AVERROR(ENOMEM);
   }
   return 0;
}

void rsEncoderDestroy(RSEncoder *encoder) {
   if (encoder->destroy != NULL) {
      encoder->destroy(encoder);
   }
   avcodec_parameters_free(&encoder->params);
}

int rsVideoEncoderCreate(RSEncoder *encoder, const AVFrame *frame) {
   int ret;
   switch (rsConfig.videoEncoder) {
   case RS_CONFIG_ENCODER_X264:
      return rsX264EncoderCreate(encoder, frame);
   case RS_CONFIG_ENCODER_VAAPI:
      return rsVaapiEncoderCreate(encoder, frame);
   }

   const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(frame->format);
   if (desc->flags & AV_PIX_FMT_FLAG_HWACCEL) {
      if ((ret = rsVaapiEncoderCreate(encoder, frame)) >= 0) {
         av_log(NULL, AV_LOG_INFO, "Created VA-API encoder\n");
         return 0;
      }
      av_log(NULL, AV_LOG_WARNING, "Failed to create VA-API encoder: %s\n",
             av_err2str(ret));

      return AVERROR(ENOSYS);
   }

   if ((ret = rsX264EncoderCreate(encoder, frame)) >= 0) {
      av_log(NULL, AV_LOG_INFO, "Created x264 encoder\n");
      return 0;
   }
   av_log(NULL, AV_LOG_WARNING, "Failed to create x264 encoder: %s\n", av_err2str(ret));

   return AVERROR(ENOSYS);
}
