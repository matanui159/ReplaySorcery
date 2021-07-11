/*
 * Copyright (C) 2020-2021  Joshua Minter, Patryk Seregiet
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

#include "abuffer.h"
#include "../config.h"
#include "../util.h"
#include "aencoder.h"

static av_always_inline void audioBufferCopy(RSAudioBuffer *buffer, void *dest,
                                             int destOffset, const void *src,
                                             int srcOffset, int size) {
   if (size >= 0) {
      memcpy((int8_t *)dest + destOffset * buffer->sampleSize,
             (const int8_t *)src + srcOffset * buffer->sampleSize,
             (size_t)(size * buffer->sampleSize));
   }
}

static int audioBufferGetEncoder(RSAudioBuffer *buffer) {
   int ret;
   if (buffer->encoder.params == NULL) {
      if ((ret = rsAudioEncoderCreate(&buffer->encoder, buffer->params)) < 0) {
         return ret;
      }
   }
   return 0;
}

static int audioBufferSendFrame(RSAudioBuffer *buffer, int index, int pts,
                                AVFrame *frame) {
   int ret;
   if (index >= buffer->size) {
      if ((ret = rsEncoderSendFrame(&buffer->encoder, NULL)) < 0) {
         return ret;
      }
      return 0;
   }

   int size = FFMIN(buffer->encoder.params->frame_size, buffer->size - index);
   frame->format = buffer->params->format;
   frame->channels = buffer->params->channels;
   frame->channel_layout = buffer->params->channel_layout;
   frame->sample_rate = buffer->params->sample_rate;
   frame->nb_samples = size;
   frame->pts = pts;
   if ((ret = av_frame_get_buffer(frame, 0)) < 0) {
      return ret;
   }

   int bufIndex = (buffer->index + index) % buffer->size;
   int prefix = FFMIN(frame->nb_samples, buffer->size - bufIndex);
   audioBufferCopy(buffer, frame->data[0], 0, buffer->data, bufIndex, prefix);
   audioBufferCopy(buffer, frame->data[0], prefix, buffer->data, 0,
                   frame->nb_samples - prefix);
   if ((ret = rsEncoderSendFrame(&buffer->encoder, frame)) < 0) {
      return ret;
   }
   return size;
}

int rsAudioBufferCreate(RSAudioBuffer *buffer, const AVCodecParameters *params) {
   int ret;
   rsClear(buffer, sizeof(RSAudioBuffer));
   buffer->params = rsParamsClone(params);
   if (buffer->params == NULL) {
      ret = AVERROR(ENOMEM);
      goto error;
   }

   buffer->sampleSize = params->channels * av_get_bytes_per_sample(params->format);
   buffer->capacity = rsConfig.recordSeconds * params->sample_rate;
   buffer->data = av_malloc_array((size_t)buffer->capacity, (size_t)buffer->sampleSize);
   if (buffer->data == NULL) {
      ret = AVERROR(ENOMEM);
      goto error;
   }

   return 0;
error:
   rsAudioBufferDestroy(buffer);
   return ret;
}

void rsAudioBufferDestroy(RSAudioBuffer *buffer) {
   rsEncoderDestroy(&buffer->encoder);
   av_freep(&buffer->data);
   avcodec_parameters_free(&buffer->params);
}

int rsAudioBufferAddFrame(RSAudioBuffer *buffer, AVFrame *frame) {
   int prefix = FFMIN(frame->nb_samples, buffer->capacity - buffer->index);
   audioBufferCopy(buffer, buffer->data, buffer->index, frame->data[0], 0, prefix);
   audioBufferCopy(buffer, buffer->data, 0, frame->data[0], prefix,
                   frame->nb_samples - prefix);
   buffer->index = (buffer->index + frame->nb_samples) % buffer->capacity;
   buffer->size = FFMIN(buffer->size + frame->nb_samples, buffer->capacity);
   buffer->endTime = frame->pts + frame->nb_samples;
   av_frame_unref(frame);
   return 0;
}

int rsAudioBufferGetParams(RSAudioBuffer *buffer, const AVCodecParameters **params) {
   int ret;
   if ((ret = audioBufferGetEncoder(buffer)) < 0) {
      return ret;
   }
   *params = buffer->encoder.params;
   return 0;
}

int rsAudioBufferWrite(RSAudioBuffer *buffer, RSOutput *output, int stream,
                       int64_t startTime) {
   int ret;
   startTime = av_rescale(startTime, buffer->params->sample_rate, AV_TIME_BASE);
   AVPacket *packet = av_packet_alloc();
   AVFrame *frame = av_frame_alloc();
   if (packet == NULL || frame == NULL) {
      ret = AVERROR(ENOMEM);
      goto error;
   }
   if ((ret = audioBufferGetEncoder(buffer)) < 0) {
      goto error;
   }

   int64_t bufStartTime = buffer->endTime - buffer->size;
   int index = (int)FFMAX(startTime - bufStartTime, 0);
   int pts = 0;
   while ((ret = rsEncoderNextPacket(&buffer->encoder, packet)) != AVERROR_EOF) {
      if (ret >= 0) {
         packet->stream_index = stream;
         if ((ret = rsOutputWrite(output, packet)) < 0) {
            goto error;
         }
      } else if (ret == AVERROR(EAGAIN)) {
         if ((ret = audioBufferSendFrame(buffer, index, pts, frame)) < 0) {
            goto error;
         }
         index += ret;
         pts += ret;
      } else {
         goto error;
      }
   }

   ret = 0;
error:
   av_frame_free(&frame);
   av_packet_free(&packet);
   rsEncoderDestroy(&buffer->encoder);
   return ret;
}
