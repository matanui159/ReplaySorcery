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

#include "swscale.h"
#include "../config.h"
#include "../util.h"

int rsSoftwareScaleCreate(RSSoftwareScale *scale, const AVCodecParameters *params,
                          enum AVPixelFormat format) {
   int ret;
   rsClear(scale, sizeof(RSSoftwareScale));
   scale->width = rsConfig.scaleWidth;
   if (scale->width == RS_CONFIG_AUTO) {
      scale->width = params->width;
   }
   scale->height = rsConfig.scaleHeight;
   if (scale->height == RS_CONFIG_AUTO) {
      scale->height = params->height;
   }

   scale->scaleCtx =
       sws_getContext(params->width, params->height, params->format, scale->width,
                      scale->height, format, 0, NULL, NULL, NULL);
   if (scale->scaleCtx == NULL) {
      av_log(NULL, AV_LOG_ERROR, "Failed to create scale context\n");
      ret = AVERROR_EXTERNAL;
      goto error;
   }

   scale->frame = av_frame_alloc();
   if (scale->frame == NULL) {
      ret = AVERROR(ENOMEM);
      goto error;
   }

   scale->frame->width = scale->width;
   scale->frame->height = scale->height;
   scale->frame->format = format;
   if ((ret = av_frame_get_buffer(scale->frame, 0)) < 0) {
      goto error;
   }

   return 0;
error:
   rsSoftwareScaleDestroy(scale);
   return ret;
}

void rsSoftwareScaleDestroy(RSSoftwareScale *scale) {
   av_frame_free(&scale->frame);
   sws_freeContext(scale->scaleCtx);
   scale->scaleCtx = NULL;
}

int rsSoftwareScale(RSSoftwareScale *scale, AVFrame *frame) {
   int ret;
   if ((ret = sws_scale(scale->scaleCtx, frame->data, frame->linesize, 0, frame->height,
                        scale->frame->data, scale->frame->linesize)) < 0) {
      goto error;
   }

   av_frame_unref(frame);
   if ((ret = av_frame_ref(frame, scale->frame)) < 0) {
      ret = AVERROR(ENOMEM);
      goto error;
   }

   return 0;
error:
   av_frame_unref(frame);
   return ret;
}
