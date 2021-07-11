/*
 * Copyright (C) 2020-2021  Joshua Minter
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

static int ffmpegEncoderFilterCreate(FFmpegEncoder *ffmpeg, AVFilterContext **filterCtx,
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

static int ffmpegEncoderFilterHardware(AVFilterContext *filterCtx,
                                       const AVBufferRef *hwFrames) {
   int ret;
   AVBufferSrcParameters *bufferParams = av_buffersrc_parameters_alloc();
   if (bufferParams == NULL) {
      ret = AVERROR(ENOMEM);
      goto error;
   }

   bufferParams->hw_frames_ctx = (AVBufferRef *)hwFrames;
   if ((ret = av_buffersrc_parameters_set(filterCtx, bufferParams)) < 0) {
      goto error;
   }

   ret = 0;
error:
   av_freep(&bufferParams);
   return ret;
}

static int ffmpegEncoderSendFrame(RSEncoder *encoder, AVFrame *frame) {
   int ret;
   FFmpegEncoder *ffmpeg = encoder->extra;
   if (frame != NULL) {
      frame->pict_type = AV_PICTURE_TYPE_NONE;
      if ((ret = av_buffersrc_add_frame(ffmpeg->sourceCtx, frame)) < 0) {
         av_log(ffmpeg->sourceCtx, AV_LOG_ERROR,
                "Failed to send frame to filter graph: %s\n", av_err2str(ret));
         goto error;
      }
      if ((ret = av_buffersink_get_frame(ffmpeg->sinkCtx, frame)) < 0) {
         av_log(ffmpeg->sinkCtx, AV_LOG_ERROR,
                "Failed to receive frame from filter graph: %s\n", av_err2str(ret));
         goto error;
      }
   }
   if ((ret = avcodec_send_frame(ffmpeg->codecCtx, frame)) < 0) {
      av_log(ffmpeg->codecCtx, AV_LOG_ERROR, "Failed to send frame to encoder: %s\n",
             av_err2str(ret));
      goto error;
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
      if (ret != AVERROR_EOF && ret != AVERROR(EAGAIN)) {
         av_log(ffmpeg->codecCtx, AV_LOG_ERROR,
                "Failed to receive packet from encoder: %s\n", av_err2str(ret));
      }
      return ret;
   }
   return 0;
}

int rsFFmpegEncoderCreate(RSEncoder *encoder, const char *name, const char *filterFmt,
                          ...) {
   int ret;
   char *filter = NULL;
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
   ffmpeg->codecCtx->flags = AV_CODEC_FLAG_GLOBAL_HEADER;
   ffmpeg->codecCtx->thread_count = 1;
   ffmpeg->codecCtx->profile = FF_PROFILE_RESERVED;

   ffmpeg->filterGraph = avfilter_graph_alloc();
   if (ffmpeg->filterGraph == NULL) {
      ret = AVERROR(ENOMEM);
      goto error;
   }

   va_list args;
   va_start(args, filterFmt);
   filter = rsFormatv(filterFmt, args);
   va_end(args);
   if (filter == NULL) {
      return AVERROR(ENOMEM);
      goto error;
   }

   av_log(ffmpeg->codecCtx, AV_LOG_INFO, "Filter graph: %s\n", filter);
   if ((ret = avfilter_graph_parse2(ffmpeg->filterGraph, filter, &ffmpeg->inputs,
                                    &ffmpeg->outputs)) < 0) {
      av_log(ffmpeg->filterGraph, AV_LOG_ERROR, "Failed to parse filter graph: %s\n",
             av_err2str(ret));
      goto error;
   }
   av_freep(&filter);

   AVFilterContext *outputCtx = ffmpeg->outputs->filter_ctx;
   int outputIndex = ffmpeg->outputs->pad_idx;
   switch (avfilter_pad_get_type(outputCtx->output_pads, outputIndex)) {
   case AVMEDIA_TYPE_VIDEO:
      ret = ffmpegEncoderFilterCreate(ffmpeg, &ffmpeg->sinkCtx, "buffersink", NULL);
      break;
   case AVMEDIA_TYPE_AUDIO:
      ret = ffmpegEncoderFilterCreate(ffmpeg, &ffmpeg->sinkCtx, "abuffersink", NULL);
      break;
   default:
      ret = AVERROR(ENOSYS);
      goto error;
   }
   if (ret < 0) {
      goto error;
   }
   if ((ret = avfilter_link(outputCtx, (unsigned)outputIndex, ffmpeg->sinkCtx, 0)) < 0) {
      av_log(ffmpeg->sinkCtx, AV_LOG_ERROR, "Failed to link output filter: %s\n",
             av_err2str(ret));
      goto error;
   }

   return 0;
error:
   av_freep(&filter);
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

int rsFFmpegEncoderOpen(RSEncoder *encoder, const AVCodecParameters *params,
                        const AVBufferRef *hwFrames) {
   int ret;
   AVDictionary *options = NULL;
   FFmpegEncoder *ffmpeg = encoder->extra;
   if ((ret = ffmpeg->error) < 0) {
      goto error;
   }

   AVFilterContext *inputCtx = ffmpeg->inputs->filter_ctx;
   int inputIndex = ffmpeg->inputs->pad_idx;
   AVRational sar = params->sample_aspect_ratio;
   switch (avfilter_pad_get_type(inputCtx->input_pads, inputIndex)) {
   case AVMEDIA_TYPE_VIDEO:
      rsOptionsSet(&options, &ret, "video_size", "%ix%i", params->width, params->height);
      rsOptionsSet(&options, &ret, "pix_fmt", "%i", params->format);
      rsOptionsSet(&options, &ret, "sar", "%i/%i", sar.num, sar.den);
      rsOptionsSet(&options, &ret, "frame_rate", "%i", rsConfig.videoFramerate);
      rsOptionsSet(&options, &ret, "time_base", "1/%i", AV_TIME_BASE);
      if (ret >= 0) {
         ret = ffmpegEncoderFilterCreate(ffmpeg, &ffmpeg->sourceCtx, "buffer", &options);
      }
      break;
   case AVMEDIA_TYPE_AUDIO:
      rsOptionsSet(&options, &ret, "sample_fmt", "%i", params->format);
      rsOptionsSet(&options, &ret, "channels", "%i", params->channels);
      rsOptionsSet(&options, &ret, "channel_layout", "0x%" PRIx64,
                   params->channel_layout);
      rsOptionsSet(&options, &ret, "sample_rate", "%i", params->sample_rate);
      rsOptionsSet(&options, &ret, "time_base", "1/%i", params->sample_rate);
      if (ret >= 0) {
         ret = ffmpegEncoderFilterCreate(ffmpeg, &ffmpeg->sourceCtx, "abuffer", &options);
      }
      break;
   default:
      ret = AVERROR(ENOSYS);
      goto error;
   }
   if (ret < 0) {
      goto error;
   }
   if (hwFrames != NULL) {
      if ((ret = ffmpegEncoderFilterHardware(ffmpeg->sourceCtx, hwFrames)) < 0) {
         goto error;
      }
   }
   if ((ret = avfilter_link(ffmpeg->sourceCtx, 0, inputCtx, (unsigned)inputIndex)) < 0) {
      av_log(ffmpeg->sourceCtx, AV_LOG_ERROR, "Failed to link input filter: %s\n",
             av_err2str(ret));
      goto error;
   }
   if ((ret = avfilter_graph_config(ffmpeg->filterGraph, ffmpeg->filterGraph)) < 0) {
      av_log(ffmpeg->filterGraph, AV_LOG_ERROR, "Failed to configure filter graph: %s\n",
             av_err2str(ret));
      goto error;
   }

   ffmpeg->codecCtx->time_base = av_buffersink_get_time_base(ffmpeg->sinkCtx);
   int format = av_buffersink_get_format(ffmpeg->sinkCtx);
   switch (ffmpeg->codecCtx->codec_type) {
   case AVMEDIA_TYPE_VIDEO:
      ffmpeg->codecCtx->width = av_buffersink_get_w(ffmpeg->sinkCtx);
      ffmpeg->codecCtx->height = av_buffersink_get_h(ffmpeg->sinkCtx);
      ffmpeg->codecCtx->pix_fmt = format;
      ffmpeg->codecCtx->sample_aspect_ratio =
          av_buffersink_get_sample_aspect_ratio(ffmpeg->sinkCtx);
      ffmpeg->codecCtx->framerate = av_buffersink_get_frame_rate(ffmpeg->sinkCtx);
      ffmpeg->codecCtx->gop_size = rsConfig.videoGOP;
      if (ffmpeg->codecCtx->profile == FF_PROFILE_RESERVED) {
         ffmpeg->codecCtx->profile = rsConfig.videoProfile;
      }
      if (rsConfig.videoBitrate != RS_CONFIG_AUTO) {
         ffmpeg->codecCtx->bit_rate = rsConfig.videoBitrate;
      }
      break;
   case AVMEDIA_TYPE_AUDIO:
      ffmpeg->codecCtx->sample_fmt = format;
      ffmpeg->codecCtx->channels = av_buffersink_get_channels(ffmpeg->sinkCtx);
      ffmpeg->codecCtx->channel_layout =
          av_buffersink_get_channel_layout(ffmpeg->sinkCtx);
      ffmpeg->codecCtx->sample_rate = av_buffersink_get_sample_rate(ffmpeg->sinkCtx);
      ffmpeg->codecCtx->profile = rsConfig.audioProfile;
      if (rsConfig.audioBitrate != RS_CONFIG_AUTO) {
         ffmpeg->codecCtx->bit_rate = rsConfig.audioBitrate;
      }
      break;
   default:
      ret = AVERROR(ENOSYS);
      goto error;
   }

   hwFrames = av_buffersink_get_hw_frames_ctx(ffmpeg->sinkCtx);
   if (hwFrames != NULL) {
      ffmpeg->codecCtx->hw_frames_ctx = av_buffer_ref((AVBufferRef *)hwFrames);
      if (ffmpeg->codecCtx->hw_frames_ctx == NULL) {
         ret = AVERROR(ENOMEM);
         goto error;
      }
   }
   if ((ret = avcodec_open2(ffmpeg->codecCtx, NULL, &ffmpeg->options)) < 0) {
      av_log(ffmpeg->codecCtx, AV_LOG_ERROR, "Failed to open encoder: %s\n",
             av_err2str(ret));
      goto error;
   }
   rsOptionsDestroy(&ffmpeg->options);

   if ((ret = avcodec_parameters_from_context(encoder->params, ffmpeg->codecCtx)) < 0) {
      goto error;
   }

   return 0;
error:
   rsOptionsDestroy(&options);
   return ret;
}
