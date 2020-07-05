/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "pktcircle.h"
#include "config.h"
#include "error.h"
#include <libavutil/avutil.h>

/**
 * A utility macro to increment the provided index while looping around the circle array.
 */
#define PACKET_CIRCLE_NEXT(pktCircle, index) index = (index + 1) % (pktCircle)->size

/**
 * Iterates all the indices in the packet circle that current contain data.
 */
#define PACKET_CIRCLE_FOREACH(pktCircle, index)                                          \
   for (size_t index = (pktCircle)->head; index != (pktCircle)->tail;                    \
        PACKET_CIRCLE_NEXT(pktCircle, index))

/**
 * Internal implementation of `rsPacketCircleClear` that does not lock the mutex. Used in
 * functions which already have locked the mutex or do not need to lock the mutex.
 */
static void packetCircleClear(RSPacketCircle* pktCircle) {
   PACKET_CIRCLE_FOREACH(pktCircle, i) {
      av_packet_unref(&pktCircle->packets[i]);
   }
   pktCircle->head = 0;
   pktCircle->tail = 0;
   pktCircle->input = pktCircle->packets;
}

/**
 * Internal implementation of `rsPacketCircleRotate` that does not lock the mutex. Used in
 * functions which already have locked the mutex.
 */
static void packetCircleRotate(RSPacketCircle* pktCircle) {
   PACKET_CIRCLE_NEXT(pktCircle, pktCircle->tail);
   pktCircle->input = &pktCircle->packets[pktCircle->tail];
   if (pktCircle->tail == pktCircle->head) {
      av_packet_unref(&pktCircle->packets[pktCircle->head]);
      PACKET_CIRCLE_NEXT(pktCircle, pktCircle->head);
   }
}

void rsPacketCircleCreate(RSPacketCircle* pktCircle) {
   // We allocate one extra packet then needed so we always have a packet we can write
   // into.
   pktCircle->size = (size_t)(rsConfig.inputFramerate * rsConfig.recordDuration + 1);
   pktCircle->packets = av_mallocz_array(pktCircle->size, sizeof(AVPacket));
   for (size_t i = 0; i < pktCircle->size; ++i) {
      av_init_packet(&pktCircle->packets[i]);
   }

   pktCircle->head = 0;
   pktCircle->tail = 0;
   pktCircle->input = pktCircle->packets;
   rsCheckPosix(pthread_mutex_init(&pktCircle->mutex, NULL));
}

void rsPacketCircleDestroy(RSPacketCircle* pktCircle) {
   pthread_mutex_destroy(&pktCircle->mutex);
   packetCircleClear(pktCircle);
   av_freep(&pktCircle->packets);
}

void rsPacketCircleRotate(RSPacketCircle* pktCircle) {
   rsCheckPosix(pthread_mutex_lock(&pktCircle->mutex));
   packetCircleRotate(pktCircle);
   rsCheckPosix(pthread_mutex_unlock(&pktCircle->mutex));
}

void rsPacketCircleClear(RSPacketCircle* pktCircle) {
   rsCheckPosix(pthread_mutex_lock(&pktCircle->mutex));
   packetCircleClear(pktCircle);
   rsCheckPosix(pthread_mutex_unlock(&pktCircle->mutex));
}

void rsPacketCircleCopy(RSPacketCircle* dst, const RSPacketCircle* src) {
   rsCheckPosix(pthread_mutex_lock(&dst->mutex));
   packetCircleClear(dst);
   rsCheckPosix(pthread_mutex_lock((pthread_mutex_t*)&src->mutex));

   // This shouldn't overflow the buffer (`tail` should equal `size` - 1 at most).
   PACKET_CIRCLE_FOREACH(src, i) {
      av_packet_ref(dst->input, &src->packets[i]);
      packetCircleRotate(dst);
   }

   rsCheckPosix(pthread_mutex_unlock((pthread_mutex_t*)&src->mutex));
   rsCheckPosix(pthread_mutex_unlock(&dst->mutex));
}
