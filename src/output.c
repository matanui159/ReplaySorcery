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
#include "rsbuild.h"
#include "util.h"
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/bprint.h>
#include <time.h>
#ifdef RS_BUILD_PTHREAD_FOUND
#include <pthread.h>
#endif

struct RSOutput {
   AVFormatContext *formatCtx;
   RSPktCircle circle;
   int refs;
   int error;
};

static void *outputThread(void *data) {
   int ret;
   RSOutput *output = data;
   if ((ret = avformat_write_header(output->formatCtx, NULL)) < 0) {
      av_log(output->formatCtx, AV_LOG_ERROR, "Failed to write header: %s\n",
             av_err2str(ret));
   }

   size_t index;
   for (index = 0; index < output->circle.size; ++index) {
      AVPacket *packet = &output->circle.packets[index];
      if (packet->flags & AV_PKT_FLAG_KEY) {
         break;
      }
   }

   int64_t offset = output->circle.packets[index].pts;
   for (; index < output->circle.size; ++index) {
      AVPacket *packet = &output->circle.packets[index];
      packet->pts -= offset;
      packet->dts -= offset;
      if ((ret = av_interleaved_write_frame(output->formatCtx, packet)) < 0) {
         av_log(output->formatCtx, AV_LOG_ERROR, "Failed to write frame: %s\n",
                av_err2str(ret));
         goto error;
      }
   }
   if ((ret = av_write_trailer(output->formatCtx)) < 0) {
      av_log(output->formatCtx, AV_LOG_ERROR, "Failed to write trailer: %s\n",
             av_err2str(ret));
   }

   ret = 0;
error:
   if (ret < 0) {
      av_log(NULL, AV_LOG_FATAL, "%s\n", av_err2str(ret));
   }
   rsOutputDestroy(&output);
   return NULL;
}

int rsOutputCreate(RSOutput **output) {
   int ret;
   char *path = NULL;
   AVBPrint buffer;
   av_bprint_init(&buffer, 0, AV_BPRINT_SIZE_UNLIMITED);
   RSOutput *out = av_mallocz(sizeof(RSOutput));
   *output = out;
   if (out == NULL) {
      ret = AVERROR(ENOMEM);
      goto error;
   }
   out->refs = 1;

   const char *configOutput = rsConfig.outputFile;
   if (configOutput[0] == '~') {
      const char *home = getenv("HOME");
      if (home == NULL) {
         av_log(NULL, AV_LOG_ERROR, "Failed to get $HOME variable\n");
         ret = AVERROR_EXTERNAL;
         goto error;
      }
      av_bprintf(&buffer, "%s", home);
      ++configOutput;
   }

   time_t timeNum = time(NULL);
   const struct tm *timeObj = localtime(&timeNum);
   av_bprint_strftime(&buffer, configOutput, timeObj);
   if (!av_bprint_is_complete(&buffer)) {
      ret = AVERROR(ENOMEM);
      goto error;
   }
   if ((ret = av_bprint_finalize(&buffer, &path)) < 0) {
      goto error;
   }
   if ((ret = avformat_alloc_output_context2(&out->formatCtx, NULL, "mp4", path)) < 0) {
      av_log(NULL, AV_LOG_ERROR, "Failed to allocate output format: %s\n",
             av_err2str(ret));
      goto error;
   }
   if ((ret = avio_open(&out->formatCtx->pb, path, AVIO_FLAG_WRITE)) < 0) {
      av_log(out->formatCtx, AV_LOG_ERROR, "Failed to open output: %s\n",
             av_err2str(ret));
      goto error;
   }
   av_freep(&path);

   return 0;
error:
   av_freep(&path);
   av_bprint_finalize(&buffer, NULL);
   rsOutputDestroy(output);
   return ret;
}

void rsOutputStream(RSOutput *output, RSEncoder *encoder) {
   if (output->error < 0) {
      return;
   }
   AVStream *stream = avformat_new_stream(output->formatCtx, NULL);
   if (stream == NULL) {
      output->error = AVERROR(ENOMEM);
      return;
   }
   output->error = avcodec_parameters_copy(stream->codecpar, encoder->params);
   stream->time_base = encoder->timebase;
}

// TODO: this function really needs a cleanup
int rsOutputRun(RSOutput *output, RSPktCircle *circle) {
   int ret;
   AVDictionary *options = NULL;
   rsOptionsSet(&options, &output->error, "movflags", "+faststart");
   if (output->error < 0) {
      return output->error;
   }
   if ((ret = avformat_init_output(output->formatCtx, &options)) < 0) {
      av_log(NULL, AV_LOG_ERROR, "Failed to setup output: %s\n", av_err2str(ret));
      return ret;
   }
   rsOptionsDestroy(&options);

   if ((ret = rsPktCircleCreate(&output->circle, circle->capacity)) < 0) {
      return ret;
   }
   if ((ret = rsPktCircleClone(&output->circle, circle)) < 0) {
      return ret;
   }

#ifdef RS_BUILD_PTHREAD_FOUND
   pthread_t thread;
   if (pthread_create(&thread, NULL, outputThread, output) == 0) {
      pthread_detach(thread);
      ++output->refs;
      return 0;
   }
#endif
   outputThread(output);
   return 0;
}

void rsOutputDestroy(RSOutput **output) {
   RSOutput *out = *output;
   if (out == NULL) {
      return;
   }
   --out->refs;
   if (out->refs > 0) {
      *output = NULL;
      return;
   }
   rsPktCircleDestroy(&out->circle);
   avformat_free_context(out->formatCtx);
   av_freep(output);
}
