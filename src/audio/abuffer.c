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

static int audioBufferSize(const AVFrame *frame, int samples) {
   return av_samples_get_buffer_size(NULL, frame->channels, samples, frame->format, 1);
}

static int audioBufferConfig(RSAudioBuffer *buffer, const AVFrame *frame) {
   int ret;
   if ((ret = audioBufferSize(frame, frame->sample_rate * rsConfig.recordSeconds)) < 0) {
      return ret;
   }

   buffer->capacity = (size_t)ret;
   buffer->data = av_malloc(buffer->capacity);
   if (buffer->data == NULL) {
      return AVERROR(ENOMEM);
   }
   return 0;
}

int rsAudioBufferCreate(RSAudioBuffer *buffer) {
   rsClear(buffer, sizeof(RSAudioBuffer));
   return 0;
}

void rsAudioBufferDestroy(RSAudioBuffer *buffer) {
   av_freep(&buffer->data);
}

int rsAudioBufferAddFrame(RSAudioBuffer *buffer, AVFrame *frame) {
   int ret;
   if (buffer->data == NULL) {
      if ((ret = audioBufferConfig(buffer, frame)) < 0) {
         goto error;
      }
   }
   if ((ret = audioBufferSize(frame, frame->nb_samples)) < 0) {
      goto error;
   }

   size_t frameSize = (size_t)ret;
   size_t startSize = FFMIN(frameSize, buffer->capacity - buffer->index);
   memcpy(buffer->data + buffer->index, frame->data[0], startSize);
   if (startSize < frameSize) {
      memcpy(buffer->data, frame->data[0] + startSize, frameSize - startSize);
   }
   buffer->index = (buffer->index + frameSize) % buffer->capacity;
   buffer->size = FFMIN(buffer->size + frameSize, buffer->capacity);

   ret = 0;
error:
   av_frame_unref(frame);
   return ret;
}
