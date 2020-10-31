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

#include "stream.h"
#include "config.h"

static av_always_inline AVPacket *streamGetPacket(RSStream *stream, size_t index) {
   if (stream->size == 0) {
      return NULL;
   }
   return &stream->packets[(stream->index + index) % stream->size];
}

int rsStreamCreate(RSStream **stream, RSEncoder *input) {
   int ret;
   RSStream *strm = av_mallocz(sizeof(RSStream));
   *stream = strm;
   if (strm == NULL) {
      ret = AVERROR(ENOMEM);
      goto error;
   }

   strm->input = input;
   strm->duration =
       av_rescale_q(rsConfig.recordSeconds, av_make_q(1, 1), input->timebase);
   av_init_packet(&strm->buffer);
   strm->capacity = 8;
   strm->packets = av_malloc_array(strm->capacity, sizeof(AVPacket));
   if (strm->packets == NULL) {
      ret = AVERROR(ENOMEM);
      goto error;
   }

#ifdef RS_BUILD_PTHREAD_FOUND
   if ((ret = pthread_mutex_init(&strm->mutex, NULL)) != 0) {
      ret = AVERROR(ret);
      av_log(NULL, AV_LOG_ERROR, "Failed to create mutex: %s\n", av_err2str(ret));
      goto error;
   }
   strm->mutexCreated = 1;
#endif

   return 0;
error:
   rsStreamDestroy(stream);
   return ret;
}

void rsStreamDestroy(RSStream **stream) {
   RSStream *strm = *stream;
   if (strm != NULL) {
#ifdef RS_BUILD_PTHREAD_FOUND
      if (strm->mutexCreated) {
         pthread_mutex_destroy(&strm->mutex);
         strm->mutexCreated = 0;
      }
#endif

      for (size_t i = 0; i < strm->size; ++i) {
         av_packet_unref(&strm->packets[i]);
      }
      strm->size = 0;
      av_freep(&strm->packets);
      av_freep(stream);
   }
}

int rsStreamUpdate(RSStream *stream) {
   int ret;
   if ((ret = rsEncoderGetPacket(stream->input, &stream->buffer)) < 0) {
      return ret;
   }

#ifdef RS_BUILD_PTHREAD_FOUND
   pthread_mutex_lock(&stream->mutex);
#endif
   int64_t start = stream->buffer.pts - stream->duration;
   // Fast code path for if we only remove one packet
   AVPacket *packet = streamGetPacket(stream, 0);
   if (packet != NULL && packet->pts < start &&
       streamGetPacket(stream, 1)->pts >= start) {
      av_packet_unref(packet);
      goto insert;
   }

   // Remove any packets from the start of the buffer that are too old
   size_t remove = 0;
   for (; remove < stream->size; ++remove) {
      AVPacket *packet = streamGetPacket(stream, remove);
      if (packet->pts >= start) {
         break;
      }
      av_packet_unref(packet);
   }

   // If we only removed one, we can use the new spot for the next packet
   // If we removed more than one, we have to decrease the size of the buffer
   size_t move = stream->size - stream->index - remove;
   if (remove > 1) {
      memmove(streamGetPacket(stream, 1), streamGetPacket(stream, remove),
              move * sizeof(AVPacket));
      stream->size -= remove - 1;
   }

   // If we did not remove any, we have to increase the size of the buffer for the next
   // packet
   if (remove == 0) {
      if (stream->size == stream->capacity) {
         stream->capacity *= 2;
         stream->packets =
             av_realloc_array(stream->packets, stream->capacity, sizeof(AVPacket));
         if (stream->packets == NULL) {
            ret = AVERROR(ENOMEM);
            goto error;
         }
      }
      ++stream->size;
      memmove(streamGetPacket(stream, 1), streamGetPacket(stream, 0),
              move * sizeof(AVPacket));
      av_init_packet(streamGetPacket(stream, 0));
   }

insert:
   av_packet_ref(streamGetPacket(stream, 0), &stream->buffer);
   av_packet_unref(&stream->buffer);
   stream->index = (stream->index + 1) % stream->size;

   ret = 0;
error:
#ifdef RS_BUILD_PTHREAD_FOUND
   pthread_mutex_unlock(&stream->mutex);
#endif
   return ret;
}
