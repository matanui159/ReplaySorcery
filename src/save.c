/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "save.h"
#include "config.h"
#include "encoder/encoder.h"
#include "error.h"
#include "pktcircle.h"
#include "record.h"
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>

static struct { RSPacketCircle pktCircle; } priv;

void rsSaveInit(void) {
   rsPacketCircleCreate(&priv.pktCircle);
}

void rsSaveExit(void) {
   rsPacketCircleDestroy(&priv.pktCircle);
}

void rsSave(void) {
   AVFormatContext* formatCtx;
   rsCheck(avformat_alloc_output_context2(&formatCtx, NULL, "mp4", rsConfig.outputFile));
   rsCheck(av_opt_set(formatCtx, "movflags", "+faststart", AV_OPT_SEARCH_CHILDREN));
   rsCheck(avio_open2(&formatCtx->pb, rsConfig.outputFile, AVIO_FLAG_WRITE, NULL, NULL));

   const RSEncoder* encoder = rsRecordVideo();
   AVStream* stream = avformat_new_stream(formatCtx, encoder->codecCtx->codec);
   avcodec_parameters_from_context(stream->codecpar, encoder->codecCtx);
   av_dump_format(formatCtx, 0, rsConfig.outputFile, 1);

   rsCheck(avformat_write_header(formatCtx, NULL));
   rsPacketCircleCopy(&priv.pktCircle, &encoder->pktCircle);
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
      rsCheck(av_write_frame(formatCtx, packet));
   }
   rsPacketCircleClear(&priv.pktCircle);
   rsCheck(av_write_trailer(formatCtx));

   avio_closep(&formatCtx->pb);
   avformat_free_context(formatCtx);
   av_log(NULL, AV_LOG_INFO, "Successfully saved!\n");
}
