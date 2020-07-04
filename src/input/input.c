/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "input.h"
#include "../error.h"
#include <string.h>
#include <libavdevice/avdevice.h>

void srInputInit(void) {
   avdevice_register_all();
}

int srInputCreate(SRInput* input, const char* name, const char* url, AVDictionary** options) {
   AVInputFormat* format = av_find_input_format(name);
   if (format == NULL) {
      av_dict_free(options);
      return AVERROR_DEMUXER_NOT_FOUND;
   }
   int ret = avformat_open_input(&input->formatCtx, url, format, options);
   av_dict_free(options);
   if (ret < 0) return ret;
   av_dump_format(input->formatCtx, 0, url, 0);

   AVCodecParameters* codecpar = input->formatCtx->streams[0]->codecpar;
   AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
   if (codec == NULL) srError(AVERROR_DECODER_NOT_FOUND);
   input->codecCtx = avcodec_alloc_context3(codec);
   srCheck(avcodec_parameters_to_context(input->codecCtx, codecpar));
   srCheck(avcodec_open2(input->codecCtx, codec, NULL));

   input->packet = av_packet_alloc();
   return 0;
}

void srInputDestroy(SRInput* input) {
   av_packet_free(&input->packet);
   avcodec_free_context(&input->codecCtx);
   avformat_close_input(&input->formatCtx);
}

void srInputRead(SRInput* input, AVFrame* frame) {
   int ret;
   while ((ret = avcodec_receive_frame(input->codecCtx, frame)) == AVERROR(EAGAIN)) {
      srCheck(av_read_frame(input->formatCtx, input->packet));
      srCheck(avcodec_send_packet(input->codecCtx, input->packet));
      av_packet_unref(input->packet);
   }
   srCheck(ret);
}
