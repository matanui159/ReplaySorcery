#ifndef SR_PKTCIRCLE_H
#define SR_PKTCIRCLE_H
#include <pthread.h>
#include <libavcodec/avcodec.h>

typedef struct SRPacketCircle {
   AVPacket* packets;
   size_t size;
   size_t head;
   size_t tail;
   pthread_mutex_t mutex;
   AVPacket* input;
} SRPacketCircle;

void srPacketCircleCreate(SRPacketCircle* pktCircle);
void srPacketCircleDestroy(SRPacketCircle* pktCircle);
void srPacketCircleRotate(SRPacketCircle* pktCircle);
void srPacketCircleClear(SRPacketCircle* pktCircle);
void srPacketCircleCopy(SRPacketCircle* dst, const SRPacketCircle* src);

#endif
