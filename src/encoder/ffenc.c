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

#include "ffenc.h"
#include "../config.h"
#include "../util.h"
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>

typedef struct FFmpegEncoder {
   AVDictionary *options;
   int error;
   AVCodecContext *codecCtx;
   AVFilterGraph *filterGraph;
   AVFilterInOut *inputs;
   AVFilterInOut *outputs;
   AVFilterContext *sourceCtx;
   AVFilterContext *sinkCtx;
} FFmpegEncoder;

static void ffmpegEncoderDestroy(RSEncoder *encoder) {
   FFmpegEncoder *ffmpeg = encoder->extra;
   if (ffmpeg != NULL) {
      avfilter_inout_free(&ffmpeg->outputs);
      avfilter_inout_free(&ffmpeg->inputs);
      avfilter_graph_free(&ffmpeg->filterGraph);
      avcodec_free_context(&ffmpeg->codecCtx);
      rsOptionsDestroy(&ffmpeg->options);
      av_freep(&encoder->extra);
   }
}

static int ffmpegEncoderCreateFilter(FFmpegEncoder *ffmpeg, AVFilterContext **filterCtx,
                                     const char *name, AVDictionary **options) {
   int ret;
   const AVFilter *filter = avfilter_get_by_name(name);
   if (filter == NULL) {
      av_log(NULL, AV_LOG_ERROR, "Filter not found: %s\n", name);
      return AVERROR_FILTER_NOT_FOUND;
   }

   *filterCtx = avfilter_graph_alloc_filter(ffmpeg->filterGraph, filter, NULL);
   if (*filterCtx == NULL) {
      return AVERROR(ENOMEM);
   }
   if ((ret = avfilter_init_dict(*filterCtx, options)) < 0) {
      av_log(*filterCtx, AV_LOG_ERROR, "Failed to setup filter: %s\n", av_err2str(ret));
      return ret;
   }
   if (options != NULL) {
      rsOptionsDestroy(options);
   }
   return 0;
}

static int ffmpegEncoderConfigFilter(FFmpegEncoder *ffmpeg, const AVFrame *frame) {
   int ret;
   AVDictionary *options = NULL;
   AVFilterContext *inputCtx = ffmpeg->inputs->filter_ctx;
   int inputIndex = ffmpeg->inputs->pad_idx;
   enum AVMediaType type = avfilter_pad_get_type(inputCtx->input_pads, inputIndex);
   if (type == AVMEDIA_TYPE_VIDEO) {
      AVRational sar = frame->sample_aspect_ratio;
      rsOptionsSet(&options, &ret, "video_size", "%ix%i", frame->width, frame->height);
      rsOptionsSet(&options, &ret, "pix_fmt", "%i", frame->format);
      rsOptionsSet(&options, &ret, "sar", "%i/%i", sar.num, sar.den);
      rsOptionsSet(&options, &ret, "frame_rate", "%i", rsConfig.videoFramerate);
      rsOptionsSet(&options, &ret, "time_base", "1/%i", AV_TIME_BASE);
      if (ret < 0) {
         goto error;
      }
      if ((ret = ffmpegEncoderCreateFilter(ffmpeg, &ffmpeg->sourceCtx, "buffer",
                                           &options)) < 0) {
         goto error;
      }
      if (frame->hw_frames_ctx != NULL) {
         AVBufferSrcParameters *params = av_buffersrc_parameters_alloc();
         if (params == NULL) {
            ret = AVERROR(ENOMEM);
            goto error;
         }
         params->hw_frames_ctx = frame->hw_frames_ctx;
         ret = av_buffersrc_parameters_set(ffmpeg->sourceCtx, params);
         av_freep(&params);
         if (ret < 0) {
            goto error;
         }
      }
   }
   if (type == AVMEDIA_TYPE_AUDIO) {
      rsOptionsSet(&options, &ret, "sample_fmt", "%i", frame->format);
      rsOptionsSet(&options, &ret, "sample_rate", "%i", frame->sample_rate);
      rsOptionsSet(&options, &ret, "channels", "%i", frame->channels);
      rsOptionsSet(&options, &ret, "channel_layout", "0x%" PRIx64, frame->channel_layout);
      rsOptionsSet(&options, &ret, "time_base", "1/%i", frame->sample_rate);
      if (ret < 0) {
         goto error;
      }
      if ((ret = ffmpegEncoderCreateFilter(ffmpeg, &ffmpeg->sourceCtx, "abuffer",
                                           &options)) < 0) {
         goto error;
      }
   }
   if ((ret = avfilter_link(ffmpeg->sourceCtx, 0, inputCtx, (unsigned)inputIndex)) < 0) {
      av_log(ffmpeg->sourceCtx, AV_LOG_ERROR, "Failed to link input filter: %s\n",
             av_err2str(ret));
      goto error;
   }

   AVFilterContext *outputCtx = ffmpeg->outputs->filter_ctx;
   int outputIndex = ffmpeg->outputs->pad_idx;
   type = avfilter_pad_get_type(outputCtx->output_pads, outputIndex);
   if (type == AVMEDIA_TYPE_VIDEO) {
      if ((ret = ffmpegEncoderCreateFilter(ffmpeg, &ffmpeg->sinkCtx, "buffersink",
                                           NULL)) < 0) {
         goto error;
      }
   }
   if (type == AVMEDIA_TYPE_AUDIO) {
      if ((ret = ffmpegEncoderCreateFilter(ffmpeg, &ffmpeg->sinkCtx, "abuffersink",
                                           NULL)) < 0) {
         goto error;
      }
   }
   if ((ret = avfilter_link(outputCtx, (unsigned)outputIndex, ffmpeg->sinkCtx, 0)) < 0) {
      av_log(ffmpeg->sinkCtx, AV_LOG_ERROR, "Failed to link output filter: %s\n",
             av_err2str(ret));
      goto error;
   }

   if ((ret = avfilter_graph_config(ffmpeg->filterGraph, NULL)) < 0) {
      av_log(ffmpeg->filterGraph, AV_LOG_ERROR, "Failed to configure filter graph: %s\n",
             av_err2str(ret));
      goto error;
   }

   return 0;
error:
   rsOptionsDestroy(&options);
   return ret;
}

static int ffmpegEncoderConfigCodec(FFmpegEncoder *ffmpeg, const AVFrame *frame) {
   int ret;
   ffmpeg->codecCtx->thread_count = 1;
   ffmpeg->codecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
   if (ffmpeg->codecCtx->codec_type == AVMEDIA_TYPE_VIDEO) {
      ffmpeg->codecCtx->width = frame->width;
      ffmpeg->codecCtx->height = frame->height;
      ffmpeg->codecCtx->pix_fmt = frame->format;
      ffmpeg->codecCtx->sample_aspect_ratio = frame->sample_aspect_ratio;
      ffmpeg->codecCtx->framerate = av_make_q(rsConfig.videoFramerate, 1);
      ffmpeg->codecCtx->time_base = AV_TIME_BASE_Q;
      ffmpeg->codecCtx->color_range = frame->color_range;
      ffmpeg->codecCtx->color_primaries = frame->color_primaries;
      ffmpeg->codecCtx->color_trc = frame->color_trc;
      ffmpeg->codecCtx->colorspace = frame->colorspace;
      ffmpeg->codecCtx->chroma_sample_location = frame->chroma_location;
      ffmpeg->codecCtx->profile = rsConfig.videoProfile;
      ffmpeg->codecCtx->gop_size = rsConfig.videoGOP;
      if (frame->hw_frames_ctx != NULL) {
         ffmpeg->codecCtx->hw_frames_ctx = av_buffer_ref(frame->hw_frames_ctx);
         if (ffmpeg->codecCtx->hw_frames_ctx == NULL) {
            return AVERROR(ENOMEM);
         }
      }
   }
   if (ffmpeg->codecCtx->codec_type == AVMEDIA_TYPE_AUDIO) {
      ffmpeg->codecCtx->sample_fmt = frame->format;
      ffmpeg->codecCtx->sample_rate = frame->sample_rate;
      ffmpeg->codecCtx->channels = frame->channels;
      ffmpeg->codecCtx->channel_layout = frame->channel_layout;
      ffmpeg->codecCtx->time_base = av_make_q(1, frame->sample_rate);
      ffmpeg->codecCtx->frame_size = frame->nb_samples;
   }
   if ((ret = avcodec_open2(ffmpeg->codecCtx, NULL, &ffmpeg->options)) < 0) {
      av_log(ffmpeg->codecCtx, AV_LOG_ERROR, "Failed to open encoder: %s\n",
             av_err2str(ret));
      return ret;
   }
   rsOptionsDestroy(&ffmpeg->options);
   return 0;
}

static int ffmpegEncoderSendFrame(RSEncoder *encoder, AVFrame *frame) {
   int ret;
   FFmpegEncoder *ffmpeg = encoder->extra;
   if (ffmpeg->sourceCtx == NULL || ffmpeg->sinkCtx == NULL) {
      if ((ret = ffmpegEncoderConfigFilter(ffmpeg, frame)) < 0) {
         goto error;
      }
   }
   if ((ret = av_buffersrc_add_frame(ffmpeg->sourceCtx, frame)) < 0) {
      av_log(ffmpeg->sourceCtx, AV_LOG_ERROR,
             "Failed to send frame to filter graph: %s\n", av_err2str(ret));
      goto error;
   }
   if ((ret = av_buffersink_get_frame(ffmpeg->sinkCtx, frame)) < 0) {
      if (ret != AVERROR(EAGAIN)) {
         av_log(ffmpeg->sinkCtx, AV_LOG_ERROR,
                "Failed to receive frame from filter graph: %s\n", av_err2str(ret));
      }
      goto error;
   }
   if (!avcodec_is_open(ffmpeg->codecCtx)) {
      if ((ret = ffmpegEncoderConfigCodec(ffmpeg, frame)) < 0) {
         goto error;
      }
      if ((ret = avcodec_parameters_from_context(encoder->params, ffmpeg->codecCtx)) <
          0) {
         goto error;
      }
   }
   if ((ret = avcodec_send_frame(ffmpeg->codecCtx, frame)) < 0) {
      av_log(ffmpeg->codecCtx, AV_LOG_ERROR, "Failed to send frame to encoder: %s\n",
             av_err2str(ret));
   }

   ret = 0;
error:
   av_frame_unref(frame);
   return ret;
}

static int ffmpegEncoderNextPacket(RSEncoder *encoder, AVPacket *packet) {
   int ret;
   FFmpegEncoder *ffmpeg = encoder->extra;
   if ((ret = avcodec_receive_packet(ffmpeg->codecCtx, packet)) < 0) {
      if (ret != AVERROR(EAGAIN)) {
         av_log(ffmpeg->codecCtx, AV_LOG_ERROR,
                "Failed to receive packet from encoder: %s\n", av_err2str(ret));
      }
      return ret;
   }
   return 0;
}

int rsFFmpegEncoderCreate(RSEncoder *encoder, const char *name) {
   int ret;
   if ((ret = rsEncoderCreate(encoder)) < 0) {
      goto error;
   }

   FFmpegEncoder *ffmpeg = av_mallocz(sizeof(FFmpegEncoder));
   encoder->extra = ffmpeg;
   encoder->destroy = ffmpegEncoderDestroy;
   encoder->sendFrame = ffmpegEncoderSendFrame;
   encoder->nextPacket = ffmpegEncoderNextPacket;
   if (ffmpeg == NULL) {
      ret = AVERROR(ENOMEM);
      goto error;
   }

   AVCodec *codec = avcodec_find_encoder_by_name(name);
   if (codec == NULL) {
      av_log(NULL, AV_LOG_ERROR, "Encoder not found: %s\n", name);
      ret = AVERROR_ENCODER_NOT_FOUND;
      goto error;
   }

   ffmpeg->codecCtx = avcodec_alloc_context3(codec);
   if (ffmpeg->codecCtx == NULL) {
      ret = AVERROR(ENOMEM);
      goto error;
   }
   ffmpeg->filterGraph = avfilter_graph_alloc();
   if (ffmpeg->filterGraph == NULL) {
      ret = AVERROR(ENOMEM);
      goto error;
   }

   return 0;
error:
   rsEncoderDestroy(encoder);
   return ret;
}

void rsFFmpegEncoderSetOption(RSEncoder *encoder, const char *key, const char *fmt, ...) {
   FFmpegEncoder *ffmpeg = encoder->extra;
   va_list args;
   va_start(args, fmt);
   rsOptionsSetv(&ffmpeg->options, &ffmpeg->error, key, fmt, args);
   va_end(args);
}

AVCodecContext *rsFFmpegEncoderGetContext(RSEncoder *encoder) {
   FFmpegEncoder *ffmpeg = encoder->extra;
   return ffmpeg->codecCtx;
}

int rsFFmpegEncoderOpen(RSEncoder *encoder, const char *filterFmt, ...) {
   int ret;
   FFmpegEncoder *ffmpeg = encoder->extra;
   if (ffmpeg->error < 0) {
      return ffmpeg->error;
   }

   va_list args;
   va_start(args, filterFmt);
   char *filter = rsFormatv(filterFmt, args);
   va_end(args);
   if (filter == NULL) {
      ret = AVERROR(ENOMEM);
      goto error;
   }
   if ((ret = avfilter_graph_parse2(ffmpeg->filterGraph, filter, &ffmpeg->inputs,
                                    &ffmpeg->outputs)) < 0) {
      av_log(ffmpeg->filterGraph, AV_LOG_ERROR, "Failed to parse filter graph: %s\n",
             av_err2str(ret));
      goto error;
   }
   if (ffmpeg->inputs == NULL || ffmpeg->outputs == NULL) {
      av_log(NULL, AV_LOG_ERROR, "Missing filter input or filter output\n");
      ret = AVERROR_FILTER_NOT_FOUND;
      goto error;
   }

   ret = 0;
error:
   av_freep(&filter);
   return ret;
}
