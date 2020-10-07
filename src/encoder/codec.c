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

#include "codec.h"
#include <libavcodec/avcodec.h>

typedef struct CodecEncoder {
   RSDevice input;
   AVCodecContext *codecCtx;
   AVFrame *frame;
} CodecEncoder;

static void codecEncoderDestroy(RSEncoder *encoder) {
   CodecEncoder *codec = encoder->extra;
   if (codec != NULL) {
      av_frame_free(&codec->frame);
      avcodec_free_context(&codec->codecCtx);
      av_freep(&encoder->extra);
   }
}

static int codecEncoderGetPacket(RSEncoder *encoder, AVPacket *packet) {
   int ret;
   CodecEncoder *codec = encoder->extra;
   while ((ret = avcodec_receive_packet(codec->codecCtx, packet)) == AVERROR(EAGAIN)) {
      if ((ret = rsDeviceGetFrame(&codec->input, codec->frame)) < 0) {
         goto error;
      }
      if ((ret = avcodec_send_frame(codec->codecCtx, codec->frame)) < 0) {
         av_log(codec->codecCtx, AV_LOG_ERROR, "Failed to send frame to encoder: %s\n",
                av_err2str(ret));
         goto error;
      }
      av_frame_unref(codec->frame);
   }

   if (ret < 0) {
      av_log(codec->codecCtx, AV_LOG_ERROR, "Failed to receive packet from encoder: %s\n",
             av_err2str(ret));
      goto error;
   }

   return 0;
error:
   av_frame_unref(codec->frame);
   return ret;
}

int rsCodecEncoderCreate(RSEncoder *encoder, const char *name, const RSDevice *input,
                         AVDictionary **options) {
   int ret;
   AVDictionary *defaultOptions = NULL;
   if (options == NULL) {
      options = &defaultOptions;
   }

   CodecEncoder *codec = av_mallocz(sizeof(CodecEncoder));
   encoder->extra = codec;
   encoder->destroy = codecEncoderDestroy;
   encoder->getPacket = codecEncoderGetPacket;
   if (codec == NULL) {
      ret = AVERROR(ENOMEM);
      goto error;
   }
   codec->input = *input;

   AVCodec *avCodec = avcodec_find_encoder_by_name(name);
   if (avCodec == NULL) {
      av_log(NULL, AV_LOG_ERROR, "Encoder not found: %s\n", name);
      ret = AVERROR_ENCODER_NOT_FOUND;
      goto error;
   }

   codec->codecCtx = avcodec_alloc_context3(avCodec);
   if (codec->codecCtx == NULL) {
      ret = AVERROR(ENOMEM);
      goto error;
   }

   codec->codecCtx->time_base = input->info.v.timebase;
   codec->codecCtx->pix_fmt = input->info.v.pixfmt;
   codec->codecCtx->width = input->info.v.width;
   codec->codecCtx->height = input->info.v.height;
   if ((ret = avcodec_open2(codec->codecCtx, avCodec, options)) < 0) {
      av_log(codec->codecCtx, AV_LOG_ERROR, "Failed to open decoder: %s\n",
             av_err2str(ret));
      goto error;
   }
   if (av_dict_count(*options) > 0) {
      const char *unused = av_dict_get(*options, "", NULL, AV_DICT_IGNORE_SUFFIX)->key;
      av_log(codec->codecCtx, AV_LOG_ERROR, "Option not found: %s\n", unused);
      ret = AVERROR_OPTION_NOT_FOUND;
      goto error;
   }
   av_dict_free(options);

   codec->frame = av_frame_alloc();
   if (codec->frame == NULL) {
      ret = AVERROR(ENOMEM);
      goto error;
   }

   return 0;
error:
   av_dict_free(options);
   rsEncoderDestroy(encoder);
   return ret;
}
