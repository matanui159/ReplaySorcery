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

#include "ffenc.h"
#include "../util.h"

int rsFFmpegEncoderCreate(RSFFmpegEncoder *ffmpeg, const char *name) {
   int ret;
   rsClear(ffmpeg, sizeof(RSFFmpegEncoder));
   AVCodec *codec = avcodec_find_encoder_by_name(name);
   if (codec == NULL) {
      av_log(NULL, AV_LOG_ERROR, "Encoder not found: %s\n", name);
      ret = AVERROR_ENCODER_NOT_FOUND;
      goto error;
   }

   ffmpeg->codecCtx = avcodec_alloc_context3(codec);
   if (ffmpeg->codecCtx == NULL) {
      ret = AVERROR(ENOMEM);
      goto error;
   }
   ffmpeg->codecCtx->time_base = AV_TIME_BASE_Q;

   return 0;
error:
   rsFFmpegEncoderDestroy(ffmpeg);
   return ret;
}

void rsFFmpegEncoderOption(RSFFmpegEncoder *ffmpeg, const char *key, const char *fmt,
                           ...) {
   va_list args;
   va_start(args, fmt);
   rsOptionsSetv(&ffmpeg->options, &ffmpeg->error, key, fmt, args);
   va_end(args);
}

int rsFFmpegEncoderOpen(RSFFmpegEncoder *ffmpeg, AVCodecParameters *params) {
   int ret;
   if ((ret = avcodec_open2(ffmpeg->codecCtx, NULL, &ffmpeg->options)) < 0) {
      av_log(ffmpeg->codecCtx, AV_LOG_ERROR, "Failed to open encoder: %s\n",
             av_err2str(ret));
      return ret;
   }
   rsOptionsDestroy(&ffmpeg->options);

   if ((ret = avcodec_parameters_from_context(params, ffmpeg->codecCtx)) < 0) {
      return ret;
   }
   av_log(ffmpeg->codecCtx, AV_LOG_INFO, "Encoded size: %ix%i\n", ffmpeg->codecCtx->width,
          ffmpeg->codecCtx->height);

   return 0;
}

void rsFFmpegEncoderDestroy(RSFFmpegEncoder *ffmpeg) {
   avcodec_free_context(&ffmpeg->codecCtx);
   rsOptionsDestroy(&ffmpeg->options);
}

int rsFFmpegEncoderSendFrame(RSFFmpegEncoder *ffmpeg, AVFrame *frame) {
   int ret;
   frame->pict_type = AV_PICTURE_TYPE_NONE;
   if ((ret = avcodec_send_frame(ffmpeg->codecCtx, frame)) < 0) {
      av_log(NULL, AV_LOG_ERROR, "Failed to send frame to encoder: %s\n",
             av_err2str(ret));
      goto error;
   }

   ret = 0;
error:
   av_frame_unref(frame);
   return ret;
}

int rsFFmpegEncoderGetPacket(RSFFmpegEncoder *ffmpeg, AVPacket *packet) {
   int ret;
   if ((ret = avcodec_receive_packet(ffmpeg->codecCtx, packet)) < 0) {
      if (ret != AVERROR(EAGAIN)) {
         av_log(NULL, AV_LOG_ERROR, "Failed to receive packet from encoder: %s\n",
                av_err2str(ret));
      }
      return ret;
   }
   return 0;
}
