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

static RSPacketList *streamPacketCreate(RSStream *stream) {
   if (stream->pool != NULL) {
      RSPacketList *plist = stream->pool;
      stream->pool = plist->next;
      plist->next = NULL;
      return plist;
   }
   RSPacketList *plist = av_mallocz(sizeof(RSPacketList));
   if (plist == NULL) {
      return NULL;
   }
   av_init_packet(&plist->packet);
   return plist;
}

static void streamPacketDestroy(RSStream *stream, RSPacketList *plist) {
   av_packet_unref(&plist->packet);
   plist->next = stream->pool;
   stream->pool = plist;
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

      RSPacketList *plist = strm->tail;
      while (plist != NULL) {
         RSPacketList *next = plist->next;
         streamPacketDestroy(strm, plist);
         plist = next;
      }
      strm->head = NULL;

      plist = strm->pool;
      while (plist != NULL) {
         RSPacketList *next = plist->next;
         av_freep(&plist);
         plist = next;
      }
      strm->pool = NULL;

      av_freep(stream);
   }
}

// TODO: probably cleaner as a linked list
int rsStreamUpdate(RSStream *stream) {
   int ret;
   RSPacketList *plist = streamPacketCreate(stream);
   if (plist == NULL) {
      return AVERROR(ENOMEM);
   }
   if ((ret = rsEncoderGetPacket(stream->input, &plist->packet)) < 0) {
      streamPacketDestroy(stream, plist);
      return ret;
   }

#ifdef RS_BUILD_PTHREAD_FOUND
   pthread_mutex_lock(&stream->mutex);
#endif
   if (stream->head == NULL) {
      stream->tail = plist;
      stream->head = plist;
   } else {
      stream->head->next = plist;
      stream->head = plist;
   }

   int64_t start = plist->packet.pts - (int64_t)rsConfig.recordSeconds * AV_TIME_BASE;
   RSPacketList *remove = stream->tail;
   while (remove != NULL && remove->packet.pts < start) {
      RSPacketList *next = remove->next;
      streamPacketDestroy(stream, remove);
      remove = next;
   }
   stream->tail = remove;

#ifdef RS_BUILD_PTHREAD_FOUND
   pthread_mutex_unlock(&stream->mutex);
#endif
   return 0;
}

AVPacket *rsStreamGetPackets(RSStream *stream, size_t *size) {
   int ret;
#ifdef RS_BUILD_PTHREAD_FOUND
   pthread_mutex_lock(&stream->mutex);
#endif
   *size = 0;
   for (RSPacketList *plist = stream->tail; plist != NULL; plist = plist->next) {
      ++*size;
   }

   AVPacket *packets = av_malloc_array(*size, sizeof(AVPacket));
   if (packets == NULL) {
      goto error;
   }

   size_t index = 0;
   for (RSPacketList *plist = stream->tail; plist != NULL; plist = plist->next) {
      if ((ret = av_packet_ref(&packets[index], &plist->packet)) < 0) {
         av_freep(&packets);
         goto error;
      }
      ++index;
   }

error:
#ifdef RS_BUILD_PTHREAD_FOUND
   pthread_mutex_unlock(&stream->mutex);
#endif
   return packets;
}

void rsStreamPacketsDestroy(AVPacket **packets, size_t size) {
   if (*packets != NULL) {
      for (size_t i = 0; i < size; ++i) {
         av_packet_unref(&(*packets)[i]);
      }
      av_freep(packets);
   }
}
