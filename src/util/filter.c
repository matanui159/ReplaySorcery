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

#include "filter.h"
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/avstring.h>
#include <libavutil/avutil.h>

static int filterLink(RSFilter *filter, AVFilterContext **filterCtx, AVFilterInOut *link,
                      const char *name, const char *options) {
   int ret;
   const AVFilter *avFilter = avfilter_get_by_name(name);
   if (avFilter == NULL) {
      av_log(filter->graph, AV_LOG_ERROR, "Failed to find filter: %s\n", name);
      return AVERROR_FILTER_NOT_FOUND;
   }
   if ((ret = avfilter_graph_create_filter(filterCtx, avFilter, NULL, options, NULL,
                                           filter->graph)) < 0) {
      av_log(filter->graph, AV_LOG_ERROR, "Failed to create filter: %s\n",
             av_err2str(ret));
      return ret;
   }
   if ((ret = avfilter_link(*filterCtx, 0, link->filter_ctx, (unsigned)link->pad_idx)) <
       0) {
      av_log(*filterCtx, AV_LOG_ERROR, "Failed to link to '%s': %s\n", link->name,
             av_err2str(ret));
      return ret;
   }
   return 0;
}

int rsFilterCreate(RSFilter *filter, const RSStreamInfo *info, const char *desc) {
   int ret;
   filter->graph = avfilter_graph_alloc();
   if (filter->graph == NULL) {
      ret = AVERROR(ENOMEM);
      goto error;
   }

   AVFilterInOut *input = NULL;
   AVFilterInOut *output = NULL;
   if ((ret = avfilter_graph_parse2(filter->graph, desc, &input, &output)) < 0) {
      av_log(filter->graph, AV_LOG_ERROR, "Failed to parse filter description: %s\n",
             av_err2str(ret));
      goto error;
   }

   char *sourceOptions =
       av_asprintf("time_base=%i/%i:pix_fmt=%i:video_size=%ix%i", info->v.timebase.num,
                   info->v.timebase.den, info->v.pixfmt, info->v.width, info->v.height);
   if (sourceOptions == NULL) {
      ret = AVERROR(ENOMEM);
      goto error;
   }
   if ((ret = filterLink(filter, &filter->sourceCtx, input, "buffer", sourceOptions)) <
       0) {
      goto error;
   }
   av_freep(&sourceOptions);
   avfilter_inout_free(&input);

   if ((ret = filterLink(filter, &filter->sourceCtx, input, "buffersink", NULL)) < 0) {
      goto error;
   }
   avfilter_inout_free(&output);

   return 0;
error:
   av_freep(&sourceOptions);
   avfilter_inout_free(&output);
   avfilter_inout_free(&input);
   rsFilterDestroy(filter);
   return ret;
}

void rsFilterDestroy(RSFilter *filter) {
   avfilter_graph_free(&filter->graph);
}

int rsFilter(RSFilter *filter, AVFrame *frame) {
   int ret;
   if ((ret = av_buffersrc_add_frame(filter->sourceCtx, frame)) < 0) {
      av_log(filter->graph, AV_LOG_ERROR, "Failed to send frame to filter: %s\n",
             av_err2str(ret));
      goto error;
   }
   if ((ret = av_buffersink_get_frame(filter->sinkCtx, frame)) < 0) {
      av_log(filter->graph, AV_LOG_ERROR, "Failed to receive frame from filter: %s\n",
             av_err2str(ret));
      goto error;
   }

   return 0;
error:
   av_frame_unref(frame);
   return ret;
}
