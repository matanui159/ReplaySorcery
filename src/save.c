/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "save.h"
#include "error.h"
#include "pktcircle.h"
#include "record.h"
#include "encoder/encoder.h"
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libavformat/avformat.h>

static struct {
   SRPacketCircle pktCircle;
} priv;

void srSaveInit(void) {
   srPacketCircleCreate(&priv.pktCircle);
}

void srSaveExit(void) {
   srPacketCircleDestroy(&priv.pktCircle);
}

void srSave(void) {
   const char* file = "recording.mp4";
   AVFormatContext* formatCtx;
   srCheck(avformat_alloc_output_context2(&formatCtx, NULL, NULL, file));
   srCheck(avio_open2(&formatCtx->pb, file, AVIO_FLAG_WRITE, NULL, NULL));
   srCheck(av_opt_set(formatCtx, "movflags", "+faststart", AV_OPT_SEARCH_CHILDREN));

   const SREncoder* encoder = srRecordVideo();
   AVStream* stream = avformat_new_stream(formatCtx, encoder->codecCtx->codec);
   stream->codecpar->codec_type = encoder->codecCtx->codec_type;
   stream->codecpar->codec_id = encoder->codecCtx->codec_id;
   stream->codecpar->width = encoder->codecCtx->width;
   stream->codecpar->height = encoder->codecCtx->height;
   stream->codecpar->format = encoder->format;
   stream->time_base = encoder->codecCtx->time_base;
   av_dump_format(formatCtx, 0, file, 1);

   srCheck(avformat_write_header(formatCtx, NULL));
   srPacketCircleCopy(&priv.pktCircle, &encoder->pktCircle);
   av_log(NULL, AV_LOG_INFO, "Saving %zu video packets...\n", priv.pktCircle.tail);
   int64_t ptsOffset = priv.pktCircle.packets[0].pts;
   if (ptsOffset == AV_NOPTS_VALUE) ptsOffset = 0;
   int64_t dtsOffset = priv.pktCircle.packets[0].dts;
   if (dtsOffset == AV_NOPTS_VALUE) dtsOffset = 0;

   for (size_t i = 0; i < priv.pktCircle.tail; ++i) {
      AVPacket* packet = &priv.pktCircle.packets[i];
      packet->pts -= ptsOffset;
      packet->dts -= dtsOffset;
      av_packet_rescale_ts(packet, encoder->codecCtx->time_base, stream->time_base);
      srCheck(av_write_frame(formatCtx, packet));
   }
   srPacketCircleClear(&priv.pktCircle);
   srCheck(av_write_trailer(formatCtx));

   avio_closep(&formatCtx->pb);
   avformat_free_context(formatCtx);
   av_log(NULL, AV_LOG_INFO, "Successfully saved!\n");
}
