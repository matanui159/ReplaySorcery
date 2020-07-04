#include "pktcircle.h"
#include "error.h"
#include <libavutil/avutil.h>

#define PACKET_CIRCLE_NEXT(pktCircle, index) index = (index + 1) % (pktCircle)->size
#define PACKET_CIRCLE_FOREACH(pktCircle, index) \
   for (size_t index = (pktCircle)->head; index != (pktCircle)->tail; PACKET_CIRCLE_NEXT(pktCircle, index))

static void packetCircleClear(SRPacketCircle* pktCircle) {
   PACKET_CIRCLE_FOREACH(pktCircle, i) {
      av_packet_unref(&pktCircle->packets[i]);
   }
   pktCircle->head = 0;
   pktCircle->tail = 0;
   pktCircle->input = &pktCircle->packets[0];
}

static void packetCircleRotate(SRPacketCircle* pktCircle) {
   PACKET_CIRCLE_NEXT(pktCircle, pktCircle->tail);
   pktCircle->input = &pktCircle->packets[pktCircle->tail];
   if (pktCircle->tail == pktCircle->head) {
      av_packet_unref(&pktCircle->packets[pktCircle->head]);
      PACKET_CIRCLE_NEXT(pktCircle, pktCircle->head);
   }
}

void srPacketCircleCreate(SRPacketCircle* pktCircle) {
   pktCircle->size = 30 * 30;
   pktCircle->packets = av_mallocz_array(pktCircle->size, sizeof(AVPacket));
   for (size_t i = 0; i < pktCircle->size; ++i) {
      av_init_packet(&pktCircle->packets[i]);
   }
   packetCircleClear(pktCircle);
   srCheckPosix(pthread_mutex_init(&pktCircle->mutex, NULL));
}

void srPacketCircleDestroy(SRPacketCircle* pktCircle) {
   pthread_mutex_destroy(&pktCircle->mutex);
   packetCircleClear(pktCircle);
   av_freep(&pktCircle->packets);
}

void srPacketCircleRotate(SRPacketCircle* pktCircle) {
   srCheckPosix(pthread_mutex_lock(&pktCircle->mutex));
   packetCircleRotate(pktCircle);
   srCheckPosix(pthread_mutex_unlock(&pktCircle->mutex));
}

void srPacketCircleClear(SRPacketCircle* pktCircle) {
   srCheckPosix(pthread_mutex_lock(&pktCircle->mutex));
   packetCircleClear(pktCircle);
   srCheckPosix(pthread_mutex_unlock(&pktCircle->mutex));
}

void srPacketCircleCopy(SRPacketCircle* dst, const SRPacketCircle* src) {
   srCheckPosix(pthread_mutex_lock(&dst->mutex));
   packetCircleClear(dst);
   srCheckPosix(pthread_mutex_lock((pthread_mutex_t*)&src->mutex));
   PACKET_CIRCLE_FOREACH(src, i) {
      av_packet_ref(dst->input, &src->packets[i]);
      packetCircleRotate(dst);
   }
   srCheckPosix(pthread_mutex_unlock((pthread_mutex_t*)&src->mutex));
   srCheckPosix(pthread_mutex_unlock(&dst->mutex));
}
