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
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/time.h>

typedef struct FFmpegDevice {
   RSDevice device;
   AVInputFormat *format;
   AVPacket packet;
   AVDictionary *options;
   int error;
   AVFormatContext *formatCtx;
   AVCodecContext *codecCtx;
} FFmpegDevice;

static void ffmpegDeviceDestroy(RSDevice *device) {
   FFmpegDevice *ffmpeg = (FFmpegDevice *)device;
   avcodec_free_context(&ffmpeg->codecCtx);
   avformat_close_input(&ffmpeg->formatCtx);
   rsOptionsDestroy(&ffmpeg->options);
}

static int ffmpegDeviceGetFrame(RSDevice *device, AVFrame *frame) {
   int ret;
   FFmpegDevice *ffmpeg = (FFmpegDevice *)device;
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

int rsFFmpegDeviceCreate(RSDevice **device, const char *name) {
   int ret;
   avdevice_register_all();
   FFmpegDevice *ffmpeg = av_mallocz(sizeof(FFmpegDevice));
   *device = &ffmpeg->device;
   if (ffmpeg == NULL) {
      ret = AVERROR(ENOMEM);
      goto error;
   }

   ffmpeg->device.timebase = AV_TIME_BASE_Q;
   ffmpeg->device.destroy = ffmpegDeviceDestroy;
   ffmpeg->device.getFrame = ffmpegDeviceGetFrame;
   ffmpeg->format = av_find_input_format(name);
   if (ffmpeg->format == NULL) {
      av_log(NULL, AV_LOG_ERROR, "Device not found: %s\n", name);
      ret = AVERROR_DEMUXER_NOT_FOUND;
      goto error;
   }
   av_init_packet(&ffmpeg->packet);

   return 0;
error:
   rsDeviceDestroy(device);
   return ret;
}

void rsFFmpegDeviceOption(RSDevice *device, const char *key, const char *fmt, ...) {
   FFmpegDevice *ffmpeg = (FFmpegDevice *)device;
   va_list args;
   va_start(args, fmt);
   rsOptionsSetv(&ffmpeg->options, &ffmpeg->error, key, fmt, args);
   va_end(args);
}

int rsFFmpegDeviceOpen(RSDevice *device, const char *input) {
   int ret;
   FFmpegDevice *ffmpeg = (FFmpegDevice *)device;
   if (ffmpeg->error < 0) {
      return ffmpeg->error;
   }
   if ((ret = avformat_open_input(&ffmpeg->formatCtx, input, ffmpeg->format,
                                  &ffmpeg->options)) < 0) {
      return ret;
   }
   rsOptionsDestroy(&ffmpeg->options);

   AVStream *stream = ffmpeg->formatCtx->streams[0];
   ffmpeg->device.params = stream->codecpar;
   AVCodec *codec = avcodec_find_decoder(stream->codecpar->codec_id);
   if (codec == NULL) {
      av_log(ffmpeg->formatCtx, AV_LOG_ERROR, "Decoder not found: %s\n",
             avcodec_get_name(stream->codecpar->codec_id));
      return AVERROR_DECODER_NOT_FOUND;
   }

   ffmpeg->codecCtx = avcodec_alloc_context3(codec);
   if (ffmpeg->codecCtx == NULL) {
      return AVERROR(ENOMEM);
   }
   if ((ret = avcodec_parameters_to_context(ffmpeg->codecCtx, stream->codecpar)) < 0) {
      return ret;
   }
   if ((ret = avcodec_open2(ffmpeg->codecCtx, codec, NULL)) < 0) {
      av_log(ffmpeg->codecCtx, AV_LOG_ERROR, "Failed to open decoder: %s\n",
             av_err2str(ret));
      return ret;
   }
   return 0;
}
