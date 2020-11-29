/*
 * Copyright (C) 2020  Joshua Minter, Patryk Seregiet
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

static int audioBufferGetEncoder(RSAudioBuffer *buffer) {
   int ret;
   if (buffer->encoder.params == NULL) {
      if ((ret = rsAudioEncoderCreate(&buffer->encoder, buffer->params)) < 0) {
         return ret;
      }
   }
   return 0;
}

int rsAudioBufferCreate(RSAudioBuffer *buffer, const AVCodecParameters *params) {
   int ret;
   rsClear(buffer, sizeof(RSAudioBuffer));
   buffer->params = avcodec_parameters_alloc();
   if (buffer->params == NULL) {
      ret = AVERROR(ENOMEM);
      goto error;
   }
   if ((ret = avcodec_parameters_copy(buffer->params, params)) < 0) {
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
   size_t prefixSize = (size_t)(prefix * buffer->sampleSize);
   memcpy(buffer->data + buffer->index * buffer->sampleSize, frame->data[0], prefixSize);
   if (prefix < frame->nb_samples) {
      size_t suffixSize = (size_t)((frame->nb_samples - prefix) * buffer->sampleSize);
      memcpy(buffer->data, frame->data[0] + prefixSize, suffixSize);
   }
   buffer->index = (buffer->index + frame->nb_samples) % buffer->capacity;
   buffer->size = FFMIN(buffer->size + frame->nb_samples, buffer->capacity);
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
                       int64_t offset) {
   int ret;
   AVPacket packet;
   av_init_packet(&packet);
   AVFrame *frame = av_frame_alloc();
   if (frame == NULL) {
      ret = AVERROR(ENOMEM);
      goto error;
   }
   if ((ret = audioBufferGetEncoder(buffer)) < 0) {
      goto error;
   }

   int realOffset = (int)av_rescale(offset, buffer->params->sample_rate, AV_TIME_BASE);
   int index = realOffset;
   while ((ret = rsEncoderNextPacket(&buffer->encoder, &packet)) != AVERROR_EOF) {
      if (ret >= 0) {
         packet.stream_index = stream;
         if ((ret = rsOutputWrite(output, &packet)) < 0) {
            goto error;
         }
      } else if (ret == AVERROR(EAGAIN)) {
         if (index == buffer->size) {
            rsEncoderSendFrame(&buffer->encoder, NULL);
         }

         frame->format = buffer->params->format;
         frame->channels = buffer->params->channels;
         frame->channel_layout = buffer->params->channel_layout;
         frame->sample_rate = buffer->params->sample_rate;
         frame->nb_samples = 1;
         frame->pts = index - realOffset;
         if ((ret = av_frame_get_buffer(frame, 0)) < 0) {
            goto error;
         }

         int realIndex = (buffer->index + index) % buffer->size;
         memcpy(frame->data[0], buffer->data + realIndex * buffer->sampleSize,
                (size_t)buffer->sampleSize);
         if ((ret = rsEncoderSendFrame(&buffer->encoder, frame)) < 0) {
            goto error;
         }
         ++index;
      } else {
         goto error;
      }
   }

   ret = 0;
error:
   av_frame_free(&frame);
   rsEncoderDestroy(&buffer->encoder);
   return ret;
}
