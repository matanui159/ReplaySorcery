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

#include "device.h"
#include "../config.h"
#include <libavdevice/avdevice.h>

void rsDeviceInit(void) {
   avdevice_register_all();
}

int rsDeviceCreate(RSDevice *device, const RSDeviceParams *params) {
   int ret;
   AVDictionary *options = NULL;
   if ((ret = av_dict_copy(&options, params->options, 0)) < 0) {
      goto error;
   }

   AVInputFormat *format = av_find_input_format(params->name);
   if (format == NULL) {
      av_log(NULL, AV_LOG_ERROR, "Device not found: %s\n", params->name);
      ret = AVERROR_DEMUXER_NOT_FOUND;
      goto error;
   }
   if ((ret = avformat_open_input(&device->formatCtx, params->input, format, &options)) <
       0) {
      av_log(NULL, AV_LOG_ERROR, "Failed to open '%s': %s\n", params->input,
             av_err2str(ret));
      goto error;
   }
   if (av_dict_count(options) > 0) {
      const char *unused = av_dict_get(options, "", NULL, AV_DICT_IGNORE_SUFFIX)->key;
      av_log(device->formatCtx, AV_LOG_ERROR, "Option not found: %s\n", unused);
      ret = AVERROR_OPTION_NOT_FOUND;
      goto error;
   }

   device->stream = device->formatCtx->streams[0];
   AVCodecParameters *codecpar = device->stream->codecpar;
   AVCodec *codec = avcodec_find_decoder(codecpar->codec_id);
   if (codec == NULL) {
      av_log(device->formatCtx, AV_LOG_ERROR, "Decoder not found: %s\n",
             avcodec_get_name(codecpar->codec_id));
      ret = AVERROR_DECODER_NOT_FOUND;
      goto error;
   }

   device->codecCtx = avcodec_alloc_context3(codec);
   if (device->codecCtx == NULL) {
      ret = AVERROR(ENOMEM);
      goto error;
   }
   if ((ret = avcodec_parameters_to_context(device->codecCtx, codecpar)) < 0) {
      goto error;
   }
   if ((ret = avcodec_open2(device->codecCtx, codec, NULL)) < 0) {
      av_log(device->codecCtx, AV_LOG_ERROR, "Failed to open decoder: %s\n",
             av_err2str(ret));
      goto error;
   }
   av_init_packet(&device->packet);

   return 0;
error:
   av_dict_free(&options);
   rsDeviceDestroy(device);
   return ret;
}

void rsDeviceDestroy(RSDevice *device) {
   avcodec_free_context(&device->codecCtx);
   avformat_close_input(&device->formatCtx);
}

int rsDeviceGetFrame(RSDevice *device, AVFrame *frame) {
   int ret;
   while ((ret = avcodec_receive_frame(device->codecCtx, frame)) == AVERROR(EAGAIN)) {
      if ((ret = av_read_frame(device->formatCtx, &device->packet)) < 0) {
         av_log(device->formatCtx, AV_LOG_ERROR, "Failed to read packet: %s\n",
                av_err2str(ret));
         return ret;
      }

      ret = avcodec_send_packet(device->codecCtx, &device->packet);
      av_packet_unref(&device->packet);
      if (ret < 0) {
         av_log(device->codecCtx, AV_LOG_ERROR, "Failed to send packet to decoder: %s\n",
                av_err2str(ret));
         return ret;
      }
   }

   if (ret < 0) {
      av_log(device->codecCtx, AV_LOG_ERROR, "Failed to receive frame from decoder: %s\n",
             av_err2str(ret));
      return ret;
   }
   return 0;
}

int rsVideoDeviceCreate(RSDevice *device) {
   int ret;
   switch (rsConfig.videoInput) {
   case RS_CONFIG_VIDEO_X11:
      return rsX11DeviceCreate(device);
   }

   if ((ret = rsX11DeviceCreate(device)) >= 0) {
      av_log(NULL, AV_LOG_INFO, "Created X11 device\n");
      return 0;
   }
   av_log(NULL, AV_LOG_WARNING, "Failed to create X11 device: %s\n", av_err2str(ret));

   return AVERROR(ENOSYS);
}
