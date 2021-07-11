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

#include "buffer.h"
#include "config.h"
#include "util.h"

static RSPacketList *bufferPacketCreate(RSBuffer *buffer) {
   if (buffer->pool != NULL) {
      RSPacketList *plist = buffer->pool;
      buffer->pool = plist->next;
      plist->next = NULL;
      return plist;
   }

   RSPacketList *plist = av_malloc(sizeof(RSPacketList));
   if (plist == NULL) {
      goto error;
   }

   plist->next = NULL;
   plist->packet = av_packet_alloc();
   if (plist->packet == NULL) {
      goto error;
   }

   return plist;
error:
   if (plist != NULL) {
      av_packet_free(&plist->packet);
      av_freep(&plist);
   }
   return NULL;
}

static void bufferPacketDestroy(RSBuffer *buffer, RSPacketList *plist) {
   av_packet_unref(plist->packet);
   plist->next = buffer->pool;
   buffer->pool = plist;
}

static RSPacketList *bufferPacketGetStart(RSBuffer *buffer) {
   for (RSPacketList *plist = buffer->tail; plist != NULL; plist = plist->next) {
      if (plist->packet->flags & AV_PKT_FLAG_KEY) {
         return plist;
      }
   }
   av_log(NULL, AV_LOG_ERROR, "No key-frame available yet\n");
   return NULL;
}

int rsBufferCreate(RSBuffer *buffer) {
   rsClear(buffer, sizeof(RSBuffer));
   return 0;
}

void rsBufferDestroy(RSBuffer *buffer) {
   RSPacketList *plist = buffer->tail;
   while (plist != NULL) {
      RSPacketList *next = plist->next;
      bufferPacketDestroy(buffer, plist);
      plist = next;
   }
   buffer->tail = NULL;

   plist = buffer->pool;
   while (plist != NULL) {
      RSPacketList *next = plist->next;
      av_packet_free(&plist->packet);
      av_freep(&plist);
      plist = next;
   }
   buffer->pool = NULL;
}

int rsBufferAddPacket(RSBuffer *buffer, AVPacket *packet) {
   RSPacketList *plist = bufferPacketCreate(buffer);
   if (plist == NULL) {
      av_packet_unref(packet);
      return AVERROR(ENOMEM);
   }
   av_packet_move_ref(plist->packet, packet);

   if (buffer->head == NULL) {
      buffer->tail = plist;
      buffer->head = plist;
   } else {
      buffer->head->next = plist;
      buffer->head = plist;
   }

   int64_t startTime = plist->packet->pts - rsConfig.recordSeconds * AV_TIME_BASE;
   RSPacketList *remove = buffer->tail;
   while (remove != NULL && remove->packet->pts < startTime) {
      RSPacketList *next = remove->next;
      bufferPacketDestroy(buffer, remove);
      remove = next;
   }
   buffer->tail = remove;
   return 0;
}

int64_t rsBufferGetStartTime(RSBuffer *buffer) {
   RSPacketList *start = bufferPacketGetStart(buffer);
   if (start == NULL) {
      return AVERROR(EAGAIN);
   }
   return start->packet->pts;
}

int rsBufferWrite(RSBuffer *buffer, RSOutput *output, int stream) {
   int ret;
   AVPacket *packet = av_packet_alloc();
   if (packet == NULL) {
      ret = AVERROR(ENOMEM);
      goto error;
   }

   RSPacketList *start = bufferPacketGetStart(buffer);
   if (start == NULL) {
      ret = AVERROR(EAGAIN);
      goto error;
   }
   for (RSPacketList *plist = start; plist != NULL; plist = plist->next) {
      if ((ret = av_packet_ref(packet, plist->packet)) < 0) {
         goto error;
      }

      packet->stream_index = stream;
      packet->pts -= start->packet->pts;
      packet->dts -= start->packet->pts;
      if ((ret = rsOutputWrite(output, packet)) < 0) {
         goto error;
      }
   }

   return 0;
error:
   av_packet_free(&packet);
   return ret;
}
