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
#include "../util.h"

static int vaapiEncoderExtradata(RSEncoder *encoder, const AVCodecParameters *params) {
   int ret;
   RSEncoder swEncoder = {0};
   AVCodecParameters *swParams = rsParamsClone(params);
   if (swParams == NULL) {
      ret = AVERROR(ENOMEM);
      goto error;
   }

   swParams->format = AV_PIX_FMT_YUV420P;
   swParams->width = encoder->params->width;
   swParams->height = encoder->params->height;
   if ((ret = rsVideoEncoderCreate(&swEncoder, swParams, NULL)) < 0) {
      goto error;
   }

   AVCodecContext *codecCtx = rsFFmpegEncoderGetContext(encoder);
   codecCtx->extradata_size = swEncoder.params->extradata_size;
   codecCtx->extradata = av_memdup(swEncoder.params->extradata, (size_t)codecCtx->extradata_size);
   if (codecCtx->extradata == NULL) {
      codecCtx->extradata_size = 0;
      ret = AVERROR(ENOMEM);
      goto error;
   }

   ret = 0;
error:
   rsEncoderDestroy(&swEncoder);
   avcodec_parameters_free(&swParams);
   return ret;
}

int rsVaapiEncoderCreate(RSEncoder *encoder, const AVCodecParameters *params,
                         const AVBufferRef *hwFrames) {
   int ret;
   int width = rsConfig.videoWidth;
   if (width == RS_CONFIG_AUTO) {
      width = params->width - rsConfig.videoX;
   }
   int height = rsConfig.videoHeight;
   if (height == RS_CONFIG_AUTO) {
      height = params->height - rsConfig.videoY;
   }
   int scaleWidth = rsConfig.scaleWidth;
   if (scaleWidth == RS_CONFIG_AUTO) {
      scaleWidth = width;
   }
   int scaleHeight = rsConfig.scaleHeight;
   if (scaleHeight == RS_CONFIG_AUTO) {
      scaleHeight = height;
   }
   if ((ret = rsFFmpegEncoderCreate(
            encoder, "h264_vaapi",
            "hwmap=derive_device=vaapi,crop=%i:%i:%i:%i,scale_vaapi=%i:%i:nv12", width,
            height, rsConfig.videoX, rsConfig.videoY, scaleWidth, scaleHeight)) < 0) {
      goto error;
   }

   AVCodecContext *codecCtx = rsFFmpegEncoderGetContext(encoder);
   codecCtx->sw_pix_fmt = AV_PIX_FMT_NV12;
   if (rsConfig.videoQuality != RS_CONFIG_AUTO) {
      codecCtx->global_quality = rsConfig.videoQuality;
      if (rsConfig.videoBitrate != RS_CONFIG_AUTO) {
         rsFFmpegEncoderSetOption(encoder, "rc_mode", "QVBR");
      } else if (rsConfig.videoPreset == RS_CONFIG_PRESET_FAST) {
         rsFFmpegEncoderSetOption(encoder, "rc_mode", "CQP");
      }
   }
   switch (rsConfig.videoPreset) {
   case RS_CONFIG_PRESET_FAST:
      codecCtx->compression_level = 6;
      rsFFmpegEncoderSetOption(encoder, "coder", "cavlc");
      break;
   case RS_CONFIG_PRESET_MEDIUM:
      codecCtx->compression_level = 4;
      break;
   case RS_CONFIG_PRESET_SLOW:
      codecCtx->compression_level = 2;
      break;
   }
   if ((ret = rsFFmpegEncoderOpen(encoder, params, hwFrames)) < 0) {
      goto error;
   }
   if (codecCtx->extradata == NULL) {
      av_log(NULL, AV_LOG_WARNING, "VAAPI encoder is missing extradata, getting from software encoder\n");
      if ((ret = vaapiEncoderExtradata(encoder, params)) < 0) {
         av_log(NULL, AV_LOG_WARNING, "Failed to get extradata from software encoder: %s\n", av_err2str(ret));
      }
   }

   return 0;
error:
   rsEncoderDestroy(encoder);
   return ret;
}
