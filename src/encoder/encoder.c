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
#include <libavutil/pixdesc.h>

int rsEncoderCreate(RSEncoder *encoder, const RSEncoderParams *params) {
   int ret;
   encoder->input = params->input;
   AVDictionary *options = NULL;
   if ((ret = av_dict_copy(&options, params->options, 0)) < 0) {
      goto error;
   }

   AVCodec *codec = avcodec_find_encoder_by_name(params->name);
   if (codec == NULL) {
      av_log(NULL, AV_LOG_ERROR, "Encoder not found: %s\n", params->name);
      ret = AVERROR_ENCODER_NOT_FOUND;
      goto error;
   }

   encoder->codecCtx = avcodec_alloc_context3(codec);
   if (encoder->codecCtx == NULL) {
      ret = AVERROR(ENOMEM);
      goto error;
   }

   int width = params->input->codecCtx->width;
   int height = params->input->codecCtx->height;
   encoder->codecCtx->pix_fmt = params->swFormat;
   encoder->codecCtx->width = width;
   encoder->codecCtx->height = height;
   encoder->codecCtx->time_base = params->input->stream->time_base;
   if ((ret = avcodec_open2(encoder->codecCtx, codec, &options)) < 0) {
      av_log(encoder->codecCtx, AV_LOG_ERROR, "Failed to open encoder: %s\n",
             av_err2str(ret));
      goto error;
   }

   encoder->frame = av_frame_alloc();
   if (encoder->frame == NULL) {
      ret = AVERROR(ENOMEM);
      goto error;
   }

   enum AVPixelFormat pixfmt = params->input->codecCtx->pix_fmt;
   if (pixfmt != params->swFormat) {
      encoder->scaleCtx =
          sws_getContext(width, height, pixfmt, width, height, params->swFormat,
                         rsConfig.videoScaler, NULL, NULL, NULL);
      if (encoder->scaleCtx == NULL) {
         av_log(encoder->codecCtx, AV_LOG_ERROR, "Failed to get scaler: %s -> %s\n",
                av_get_pix_fmt_name(pixfmt), av_get_pix_fmt_name(params->swFormat));
         ret = AVERROR_EXTERNAL;
         goto error;
      }

      encoder->scaleFrame = av_frame_alloc();
      if (encoder->scaleFrame == NULL) {
         ret = AVERROR(ENOMEM);
         goto error;
      }

      encoder->scaleFrame->format = params->swFormat;
      encoder->scaleFrame->width = width;
      encoder->scaleFrame->height = height;
      if ((ret = av_frame_get_buffer(encoder->scaleFrame, 0)) < 0) {
         goto error;
      }
   }

   return 0;
error:
   av_dict_free(&options);
   rsEncoderDestroy(encoder);
   return ret;
}

void rsEncoderDestroy(RSEncoder *encoder) {
   av_frame_free(&encoder->scaleFrame);
   sws_freeContext(encoder->scaleCtx);
   encoder->scaleCtx = NULL;
   av_frame_free(&encoder->frame);
   avcodec_free_context(&encoder->codecCtx);
}

int rsEncoderGetPacket(RSEncoder *encoder, AVPacket *packet) {
   int ret;
   while ((ret = avcodec_receive_packet(encoder->codecCtx, packet)) == AVERROR(EAGAIN)) {
      if ((ret = rsDeviceGetFrame(encoder->input, encoder->frame)) < 0) {
         return ret;
      }
      AVFrame *frame = encoder->frame;

      if (encoder->scaleCtx != NULL) {
         if ((ret = sws_scale(encoder->scaleCtx, (const uint8_t **)frame->data,
                              frame->linesize, 0, frame->height,
                              encoder->scaleFrame->data, encoder->scaleFrame->linesize)) <
             0) {
            av_log(encoder->scaleCtx, AV_LOG_ERROR, "Failed to scale frame: %s\n",
                   av_err2str(ret));
            return ret;
         }
         frame = encoder->scaleFrame;
      }

      if ((ret = avcodec_send_frame(encoder->codecCtx, frame)) < 0) {
         av_log(encoder->codecCtx, AV_LOG_ERROR, "Failed to send frame to encoder: %s\n",
                av_err2str(ret));
         return ret;
      }
   }

   if (ret < 0) {
      av_log(encoder->codecCtx, AV_LOG_ERROR,
             "Failed to receive packet from encoder: %s\n", av_err2str(ret));
      return ret;
   }
   return 0;
}

int rsVideoEncoderCreate(RSEncoder *encoder, RSDevice *input) {
   int ret;
   switch (rsConfig.videoEncoder) {
   case RS_CONFIG_VIDEO_X264:
      return rsX264EncoderCreate(encoder, input);
   }

   if ((ret = rsX264EncoderCreate(encoder, input)) >= 0) {
      return 0;
   }
   av_log(NULL, AV_LOG_WARNING, "Failed to create x264 encoder: %s\n", av_err2str(ret));

   return AVERROR(ENOSYS);
}
