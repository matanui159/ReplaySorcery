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

#include "output.h"
#include "config.h"
#include "util.h"
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/bprint.h>
#include <time.h>

static int outputStream(AVFormatContext *formatCtx, RSStream *stream) {
   int ret;
   AVStream *formatStream = avformat_new_stream(formatCtx, NULL);
   if (formatStream == NULL) {
      return AVERROR(ENOMEM);
   }
   if ((ret = avcodec_parameters_copy(formatStream->codecpar, stream->input->params)) <
       0) {
      return ret;
   }
   formatStream->time_base = AV_TIME_BASE_Q;
   return 0;
}

int rsOutput(RSStream *videoStream) {
   int ret;
   AVBPrint buffer;
   av_bprint_init(&buffer, 0, AV_BPRINT_SIZE_UNLIMITED);
   char *path = NULL;
   AVFormatContext *formatCtx = NULL;
   AVDictionary *options = NULL;
   AVPacket *videoPackets = NULL;

   const char *outputFile = rsConfig.outputFile;
   if (outputFile[0] == '~') {
      const char *home = getenv("HOME");
      if (home == NULL) {
         av_log(NULL, AV_LOG_ERROR, "Failed to get $HOME variable\n");
         ret = AVERROR_EXTERNAL;
         goto error;
      }
      av_bprintf(&buffer, "%s", home);
      ++outputFile;
   }

   time_t timeNum = time(NULL);
   struct tm *timeObj = localtime(&timeNum);
   av_bprint_strftime(&buffer, outputFile, timeObj);
   if (!av_bprint_is_complete(&buffer)) {
      ret = AVERROR(ENOMEM);
      goto error;
   }
   if ((ret = av_bprint_finalize(&buffer, &path)) < 0) {
      goto error;
   }

   av_log(NULL, AV_LOG_INFO, "Saving video to '%s'...\n", path);
   if ((ret = avformat_alloc_output_context2(&formatCtx, NULL, "mp4", path)) < 0) {
      av_log(NULL, AV_LOG_ERROR, "Failed to allocate output format: %s\n",
             av_err2str(ret));
      goto error;
   }
   if ((ret = avio_open(&formatCtx->pb, path, AVIO_FLAG_WRITE)) < 0) {
      av_log(formatCtx, AV_LOG_ERROR, "Failed to open output: %s\n", av_err2str(ret));
      goto error;
   }
   av_freep(&path);

   if ((ret = outputStream(formatCtx, videoStream)) < 0) {
      goto error;
   }

   rsOptionsSet(&options, &ret, "movflags", "+faststart");
   if (ret < 0) {
      goto error;
   }
   if ((ret = avformat_init_output(formatCtx, &options)) < 0) {
      av_log(formatCtx, AV_LOG_ERROR, "Failed to setup output format: %s\n",
             av_err2str(ret));
      goto error;
   }
   rsOptionsDestroy(&options);

   if ((ret = avformat_write_header(formatCtx, NULL)) < 0) {
      av_log(formatCtx, AV_LOG_ERROR, "Failed to write header: %s\n", av_err2str(ret));
      goto error;
   }

   size_t videoSize;
   videoPackets = rsStreamGetPackets(videoStream, &videoSize);
   if (videoPackets == NULL) {
      ret = AVERROR(ENOMEM);
      goto error;
   }

   size_t index = 0;
   for (; index < videoSize; ++index) {
      if (videoPackets[index].flags & AV_PKT_FLAG_KEY) {
         break;
      }
   }

   int64_t startTime = videoPackets[index].pts;
   for (; index < videoSize; ++index) {
      AVPacket *packet = &videoPackets[index];
      packet->pts -= startTime;
      packet->dts -= startTime;
      if ((ret = av_interleaved_write_frame(formatCtx, packet)) < 0) {
         av_log(formatCtx, AV_LOG_ERROR, "Failed to write frame: %s\n", av_err2str(ret));
         goto error;
      }
   }
   av_freep(&videoPackets);

   if ((ret = av_write_trailer(formatCtx)) < 0) {
      av_log(formatCtx, AV_LOG_ERROR, "Failed to write trailer: %s\n", av_err2str(ret));
      goto error;
   }

   return 0;
error:
   av_freep(&videoPackets);
   rsOptionsDestroy(&options);
   avformat_free_context(formatCtx);
   av_freep(&path);
   av_bprint_finalize(&buffer, NULL);
   return ret;
}
