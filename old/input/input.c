/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "input.h"
#include "../error.h"
#include <libavdevice/avdevice.h>
#include <string.h>

void rsInputInit(void) {
   avdevice_register_all();
}

int rsInputCreate(RSInput *input, const char *name, const char *url,
                  AVDictionary **options) {
   AVInputFormat *format = av_find_input_format(name);
   if (format == NULL) {
      av_dict_free(options);
      return AVERROR_DEMUXER_NOT_FOUND;
   }
   int ret = avformat_open_input(&input->formatCtx, url, format, options);
   av_dict_free(options);
   if (ret < 0)
      return ret;
   av_dump_format(input->formatCtx, 0, url, 0);

   // From this point on we can assume that the format is supported and no longer need to
   // worry about safely returning on failure.
   // We assume that the input device only has one stream.
   AVCodecParameters *codecpar = input->formatCtx->streams[0]->codecpar;
   AVCodec *codec = avcodec_find_decoder(codecpar->codec_id);
   if (codec == NULL)
      rsError(AVERROR_DECODER_NOT_FOUND);
   input->codecCtx = avcodec_alloc_context3(codec);

   // Copy the parameters from the format stream to configure the decoder.
   rsCheck(avcodec_parameters_to_context(input->codecCtx, codecpar));
   rsCheck(avcodec_open2(input->codecCtx, codec, NULL));
   input->packet = av_packet_alloc();
   return 0;
}

void rsInputDestroy(RSInput *input) {
   av_packet_free(&input->packet);
   avcodec_free_context(&input->codecCtx);
   avformat_close_input(&input->formatCtx);
}

void rsInputRead(RSInput *input, AVFrame *frame) {
   int ret;
   // Multiple packets might be needed or the last packet might still have enough data to
   // output another frame.
   while ((ret = avcodec_receive_frame(input->codecCtx, frame)) == AVERROR(EAGAIN)) {
      rsCheck(av_read_frame(input->formatCtx, input->packet));
      rsCheck(avcodec_send_packet(input->codecCtx, input->packet));
      av_packet_unref(input->packet);
   }
   rsCheck(ret);
}
