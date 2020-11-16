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

#include "ffdev.h"
#include "../util.h"
#include <libavdevice/avdevice.h>
#include <libavutil/avutil.h>
#include <libavutil/time.h>

static void ffmpegDeviceDestroy(RSDevice *device) {
   rsFFmpegDeviceDestroy(device->extra);
}

static int ffmpegDeviceGetFrame(RSDevice *device, AVFrame *frame) {
   return rsFFmpegDeviceGetFrame(device->extra, frame);
}

int rsFFmpegDeviceCreate(RSFFmpegDevice *ffmpeg, const char *name) {
   int ret;
   avdevice_register_all();
   rsClear(ffmpeg, sizeof(RSFFmpegDevice));
   ffmpeg->format = av_find_input_format(name);
   if (ffmpeg->format == NULL) {
      av_log(NULL, AV_LOG_ERROR, "Device not found: %s\n", name);
      ret = AVERROR_DEMUXER_NOT_FOUND;
      goto error;
   }
   av_init_packet(&ffmpeg->packet);

   return 0;
error:
   rsFFmpegDeviceDestroy(ffmpeg);
   return ret;
}

void rsFFmpegDeviceOption(RSFFmpegDevice *ffmpeg, const char *key, const char *fmt, ...) {
   va_list args;
   va_start(args, fmt);
   rsOptionsSetv(&ffmpeg->options, &ffmpeg->error, key, fmt, args);
   va_end(args);
}

int rsFFmpegDeviceOpen(RSFFmpegDevice *ffmpeg, const char *input,
                       AVCodecParameters *params) {
   int ret;
   if (ffmpeg->error < 0) {
      return ffmpeg->error;
   }
   if ((ret = avformat_open_input(&ffmpeg->formatCtx, input, ffmpeg->format,
                                  &ffmpeg->options)) < 0) {
      return ret;
   }
   rsOptionsDestroy(&ffmpeg->options);

   AVCodecParameters *codecpar = ffmpeg->formatCtx->streams[0]->codecpar;
   AVCodec *codec = avcodec_find_decoder(codecpar->codec_id);
   if (codec == NULL) {
      av_log(ffmpeg->formatCtx, AV_LOG_ERROR, "Decoder not found: %s\n",
             avcodec_get_name(codecpar->codec_id));
      return AVERROR_DECODER_NOT_FOUND;
   }

   ffmpeg->codecCtx = avcodec_alloc_context3(codec);
   if (ffmpeg->codecCtx == NULL) {
      return AVERROR(ENOMEM);
   }
   if ((ret = avcodec_parameters_to_context(ffmpeg->codecCtx, codecpar)) < 0) {
      return ret;
   }
   if ((ret = avcodec_open2(ffmpeg->codecCtx, codec, NULL)) < 0) {
      av_log(ffmpeg->codecCtx, AV_LOG_ERROR, "Failed to open decoder: %s\n",
             av_err2str(ret));
      return ret;
   }
   if ((ret = avcodec_parameters_from_context(params, ffmpeg->codecCtx)) < 0) {
      return ret;
   }
   return 0;
}

void rsFFmpegDeviceDestroy(RSFFmpegDevice *ffmpeg) {
   avcodec_free_context(&ffmpeg->codecCtx);
   avformat_close_input(&ffmpeg->formatCtx);
   rsOptionsDestroy(&ffmpeg->options);
}

int rsFFmpegDeviceGetFrame(RSFFmpegDevice *ffmpeg, AVFrame *frame) {
   int ret;
   int64_t pts = av_gettime_relative();
   while ((ret = avcodec_receive_frame(ffmpeg->codecCtx, frame)) == AVERROR(EAGAIN)) {
      if ((ret = av_read_frame(ffmpeg->formatCtx, &ffmpeg->packet)) < 0) {
         av_log(ffmpeg->formatCtx, AV_LOG_ERROR, "Failed to read frame: %s\n",
                av_err2str(ret));
         return ret;
      }

      ret = avcodec_send_packet(ffmpeg->codecCtx, &ffmpeg->packet);
      av_packet_unref(&ffmpeg->packet);
      if (ret < 0) {
         av_log(ffmpeg->codecCtx, AV_LOG_ERROR, "Failed to send packet to decoder: %s\n",
                av_err2str(ret));
         return ret;
      }
   }

   if (ret < 0) {
      av_log(ffmpeg->codecCtx, AV_LOG_ERROR, "Failed to receive frame from decoder: %s\n",
             av_err2str(ret));
   }
   frame->pts = pts;
   return 0;
}

int rsFFmpegDeviceWrap(RSDevice *device, const char *name) {
   int ret;
   if ((ret = rsDeviceCreate(device)) < 0) {
      goto error;
   }

   device->extra = av_malloc(sizeof(RSFFmpegDevice));
   if (device->extra == NULL) {
      ret = AVERROR(ENOMEM);
      goto error;
   }

   device->destroy = ffmpegDeviceDestroy;
   device->getFrame = ffmpegDeviceGetFrame;
   if ((ret = rsFFmpegDeviceCreate(device->extra, name)) < 0) {
      goto error;
   }

   return 0;
error:
   rsDeviceDestroy(device);
   return ret;
}
