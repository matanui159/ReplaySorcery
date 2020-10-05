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

#include "demux.h"
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

typedef struct DemuxDevice {
   AVFormatContext *formatCtx;
   AVCodecContext *codecCtx;
   AVPacket packet;
} DemuxDevice;

static void demuxDeviceDestroy(RSDevice *device) {
   DemuxDevice *demux = device->extra;
   if (demux != NULL) {
      avcodec_free_context(&demux->codecCtx);
      avformat_close_input(&demux->formatCtx);
      av_freep(&device->extra);
   }
}

static int demuxDeviceGetFrame(RSDevice *device, AVFrame *frame) {
   int ret;
   DemuxDevice *demux = device->extra;
   while ((ret = avcodec_receive_frame(demux->codecCtx, frame)) == AVERROR(EAGAIN)) {
      if ((ret = av_read_frame(demux->formatCtx, &demux->packet)) < 0) {
         av_log(demux->formatCtx, AV_LOG_ERROR, "Failed to read frame: %s\n",
                av_err2str(ret));
         return ret;
      }
      if ((ret = avcodec_send_packet(demux->codecCtx, &demux->packet)) < 0) {
         av_log(demux->codecCtx, AV_LOG_ERROR, "Failed to send packet to decoder: %s\n",
                av_err2str(ret));
         return ret;
      }
   }
   if (ret < 0) {
      av_log(demux->codecCtx, AV_LOG_ERROR, "Failed to receive packet from decoder: %s\n",
             av_err2str(ret));
      return ret;
   }
   return 0;
}

int rsDemuxDeviceCreate(RSDevice *device, const char *name, const char *input,
                        AVDictionary **options) {
   int ret;
   AVDictionary *defaultOptions = NULL;
   if (options == NULL) {
      options = &defaultOptions;
   }

   DemuxDevice *demux = av_mallocz(sizeof(DemuxDevice));
   device->extra = demux;
   device->destroy = demuxDeviceDestroy;
   device->getFrame = demuxDeviceGetFrame;
   if (demux == NULL) {
      ret = AVERROR(ENOMEM);
      goto error;
   }

   AVInputFormat *format = av_find_input_format(name);
   if (format == NULL) {
      av_log(NULL, AV_LOG_ERROR, "Device not found: %s\n", name);
      ret = AVERROR_DEMUXER_NOT_FOUND;
      goto error;
   }
   if ((ret = avformat_open_input(&demux->formatCtx, input, format, options)) < 0) {
      av_log(demux->formatCtx, AV_LOG_ERROR, "Failed to open '%s': %s\n", input,
             av_err2str(ret));
      goto error;
   }
   if (av_dict_count(*options) > 0) {
      const char *unused = av_dict_get(*options, "", NULL, AV_DICT_IGNORE_SUFFIX)->key;
      av_log(demux->formatCtx, AV_LOG_ERROR, "Option not found: %s\n", unused);
      ret = AVERROR_OPTION_NOT_FOUND;
      goto error;
   }
   av_dict_free(options);

   AVStream *stream = demux->formatCtx->streams[0];
   AVCodec *codec = avcodec_find_decoder(stream->codecpar->codec_id);
   if (codec == NULL) {
      av_log(demux->formatCtx, AV_LOG_ERROR, "Decoder not found: %s\n",
             avcodec_get_name(stream->codecpar->codec_id));
      ret = AVERROR_DECODER_NOT_FOUND;
      goto error;
   }

   demux->codecCtx = avcodec_alloc_context3(codec);
   if (demux->codecCtx == NULL) {
      ret = AVERROR(ENOMEM);
      goto error;
   }
   if ((ret = avcodec_parameters_to_context(demux->codecCtx, stream->codecpar)) < 0) {
      goto error;
   }
   if ((ret = avcodec_open2(demux->codecCtx, codec, NULL)) < 0) {
      av_log(demux->codecCtx, AV_LOG_ERROR, "Failed to open decoder: %s\n",
             av_err2str(ret));
      goto error;
   }

   av_init_packet(&demux->packet);
   device->info.pixfmt = demux->codecCtx->pix_fmt;
   device->info.width = demux->codecCtx->width;
   device->info.height = demux->codecCtx->height;
   device->info.timebase = stream->time_base;
   device->info.framerate = stream->r_frame_rate;
   return 0;

error:
   av_dict_free(options);
   rsDeviceDestroy(device);
   return ret;
}
