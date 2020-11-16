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
#include "swscale.h"

typedef struct X264Encoder {
   RSSoftwareScale scale;
   RSFFmpegEncoder ffmpeg;
} X264Encoder;

static void x264EncoderDestroy(RSEncoder *encoder) {
   X264Encoder *x264 = encoder->extra;
   rsFFmpegEncoderDestroy(&x264->ffmpeg);
   rsSoftwareScaleDestroy(&x264->scale);
}

static int x264EncoderSendFrame(RSEncoder *encoder, AVFrame *frame) {
   int ret;
   X264Encoder *x264 = encoder->extra;
   if ((ret = rsSoftwareScale(&x264->scale, frame)) < 0) {
      return ret;
   }
   if ((ret = rsFFmpegEncoderSendFrame(&x264->ffmpeg, frame)) < 0) {
      return ret;
   }
   return 0;
}

static int x264EncoderGetPacket(RSEncoder *encoder, AVPacket *packet) {
   X264Encoder *x264 = encoder->extra;
   return rsFFmpegEncoderGetPacket(&x264->ffmpeg, packet);
}

int rsX264EncoderCreate(RSEncoder *encoder, const AVCodecParameters *params) {
   int ret;
   if ((ret = rsEncoderCreate(encoder)) < 0) {
      goto error;
   }

   X264Encoder *x264 = av_mallocz(sizeof(X264Encoder));
   encoder->extra = x264;
   if (encoder->extra == NULL) {
      ret = AVERROR(ENOMEM);
      goto error;
   }

   encoder->destroy = x264EncoderDestroy;
   encoder->sendFrame = x264EncoderSendFrame;
   encoder->getPacket = x264EncoderGetPacket;
   if ((ret = rsSoftwareScaleCreate(&x264->scale, params, AV_PIX_FMT_YUV420P)) < 0) {
      goto error;
   }
   if ((ret = rsFFmpegEncoderCreate(&x264->ffmpeg, "libx264")) < 0) {
      goto error;
   }

   AVCodecContext *codecCtx = x264->ffmpeg.codecCtx;
   codecCtx->width = x264->scale.width;
   codecCtx->height = x264->scale.height;
   codecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
   codecCtx->framerate = av_make_q(rsConfig.videoFramerate, 1);
   codecCtx->profile = rsConfig.videoProfile;
   codecCtx->gop_size = rsConfig.videoGOP;
   codecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
   codecCtx->thread_count = 1;
   rsFFmpegEncoderOption(&x264->ffmpeg, "forced-idr", "true");
   if (rsConfig.videoQuality != RS_CONFIG_AUTO) {
      rsFFmpegEncoderOption(&x264->ffmpeg, "qp", "%i", rsConfig.videoQuality);
   }
   switch (rsConfig.videoPreset) {
   case RS_CONFIG_PRESET_FAST:
      rsFFmpegEncoderOption(&x264->ffmpeg, "preset", "ultrafast");
      break;
   case RS_CONFIG_PRESET_MEDIUM:
      rsFFmpegEncoderOption(&x264->ffmpeg, "preset", "medium");
      break;
   case RS_CONFIG_PRESET_SLOW:
      rsFFmpegEncoderOption(&x264->ffmpeg, "preset", "slower");
      break;
   }
   if ((ret = rsFFmpegEncoderOpen(&x264->ffmpeg, encoder->params)) < 0) {
      goto error;
   }

   return 0;
error:
   rsEncoderDestroy(encoder);
   return ret;
}
