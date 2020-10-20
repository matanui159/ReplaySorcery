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
#include <libavutil/avstring.h>
#include <libavutil/bprint.h>
#include <time.h>

static int outputRun(RSOutput *output) {
   int ret;
   size_t end = output->index + (size_t)rsConfig.recordSeconds;
   if (end > output->videoCircle.size) {
      end = output->videoCircle.size;
   }
   for (; output->index < end; ++output->index) {
      AVPacket *packet = &output->videoCircle.packets[output->index];
      if ((ret = av_interleaved_write_frame(output->formatCtx, packet)) < 0) {
         av_log(output->formatCtx, AV_LOG_ERROR, "Failed to write frame: %s\n",
                av_err2str(ret));
         goto error;
      }
   }
   if (end == output->videoCircle.size) {
      if ((ret = av_write_trailer(output->formatCtx)) < 0) {
         av_log(output->formatCtx, AV_LOG_ERROR, "Failed to write trailer: %s\n",
                av_err2str(ret));
         goto error;
      }
      ret = 0;
   } else {
      ret = AVERROR(EAGAIN);
   }

error:
   if (output->ret != ret) {
      output->ret = ret;
   }
   return ret;
}

#ifdef RS_BUILD_PTHREAD_FOUND
static void *outputThread(void *extra) {
   int ret;
   RSOutput *output = extra;
   while ((ret = outputRun(output)) == AVERROR(EAGAIN)) {
   }
   return NULL;
}
#endif

int rsOutputCreate(RSOutput *output, const RSOutputParams *params) {
   int ret;
   AVDictionary *options = NULL;
   AVBPrint buffer;
   av_bprint_init(&buffer, 0, AV_BPRINT_SIZE_UNLIMITED);
   char *path = NULL;
   av_dict_set(&options, "movflags", "+faststart", 0);
   if (av_dict_count(options) != 1) {
      ret = AVERROR(ENOMEM);
      goto error;
   }

   const char *configPath = rsConfig.outputPath;
   if (configPath[0] == '~') {
      const char *home = getenv("HOME");
      if (home == NULL) {
         av_log(NULL, AV_LOG_ERROR, "Could not find $HOME variable\n");
         ret = AVERROR_EXTERNAL;
         goto error;
      }
      av_bprintf(&buffer, "%s", home);
      ++configPath;
   }

   time_t timeNum = time(NULL);
   const struct tm *timeObj = localtime(&timeNum);
   av_bprint_strftime(&buffer, configPath, timeObj);
   if (!av_bprint_is_complete(&buffer)) {
      ret = AVERROR(ENOMEM);
      goto error;
   }
   if ((ret = av_bprint_finalize(&buffer, &path)) < 0) {
      goto error;
   }
   if ((ret = avformat_alloc_output_context2(&output->formatCtx, NULL, "mp4", path)) <
       0) {
      av_log(NULL, AV_LOG_ERROR, "Failed to allocate output format: %s\n",
             av_err2str(ret));
      goto error;
   }
   if ((ret = avio_open(&output->formatCtx->pb, path, AVIO_FLAG_WRITE)) < 0) {
      av_log(output->formatCtx, AV_LOG_ERROR, "Failed to open output: %s\n",
             av_err2str(ret));
      goto error;
   }
   av_freep(&path);

   AVStream *stream = avformat_new_stream(output->formatCtx, NULL);
   if (stream == NULL) {
      ret = AVERROR(ENOMEM);
      goto error;
   }
   if ((ret = avcodec_parameters_from_context(stream->codecpar,
                                              params->videoEncoder->codecCtx)) < 0) {
      goto error;
   }
   if ((ret = avformat_init_output(output->formatCtx, &options)) < 0) {
      av_log(output->formatCtx, AV_LOG_ERROR, "Failed to open output format: %s\n",
             av_err2str(ret));
      goto error;
   }
   av_dict_free(&options);

   if ((ret = avformat_write_header(output->formatCtx, NULL)) < 0) {
      av_log(output->formatCtx, AV_LOG_ERROR, "Failed to write header: %s\n",
             av_err2str(ret));
      goto error;
   }
   if ((ret = rsPktCircleCreate(&output->videoCircle, params->videoCircle->capacity)) <
       0) {
      goto error;
   }
   if ((ret = rsPktCircleClone(&output->videoCircle, params->videoCircle)) < 0) {
      av_log(NULL, AV_LOG_ERROR, "Failed to clone packet circle: %s\n", av_err2str(ret));
      goto error;
   }

   int64_t offset = output->videoCircle.packets[0].pts;
   for (size_t i = 0; i < output->videoCircle.size; ++i) {
      output->videoCircle.packets[0].pts -= offset;
   }

#ifdef RS_BUILD_PTHREAD_FOUND
   if ((ret = pthread_create(&output->thread, NULL, outputThread, output)) != 0) {
      av_log(NULL, AV_LOG_WARNING, "Failed to create thread: %s\n",
             av_err2str(AVERROR(ret)));
      output->thread = 0;
   }
#else
   av_log(NULL, AV_LOG_WARNING, "PThreads was not found during compilaton\n");
#endif

   return 0;
error:
   av_freep(&path);
   av_bprint_finalize(&buffer, NULL);
   av_dict_free(&options);
   rsOutputDestroy(output);
   return ret;
}

void rsOutputDestroy(RSOutput *output) {
#ifdef RS_BUILD_PTHREAD_FOUND
   if (output->thread != 0) {
      pthread_join(output->thread, NULL);
   }
#endif
   rsPktCircleDestroy(&output->videoCircle);
   avformat_free_context(output->formatCtx);
   *output = (RSOutput)RS_OUTPUT_INIT;
}

int rsOutputRun(RSOutput *output) {
#ifdef RS_BUILD_PTHREAD_FOUND
   if (output->thread != 0) {
      return output->ret;
   }
#endif
   return outputRun(output);
}
