/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef SR_PKTCIRCLE_H
#define SR_PKTCIRCLE_H
#include <pthread.h>
#include <libavcodec/avcodec.h>

/**
 * A circle buffer of packets. Allows reading making copies from other threads while one
 * thread is writing to it. Note that for performance and code simplicity only one thread
 * can write to it at a time. Also note that the creation and destruction functions are in
 * no way multi-thread safe.
 * 
 * To write to the packet circle, write into `input` and then call `srPacketCircleRotate`
 * which will change what `input` is pointing to. Repeat.
 *
 * To read from the packet circle that no other threads are writing to, directly index
 * into the `packets` array. If another thread is writing into the packet buffer, create
 * your own packet buffer and copy the data with `srPacketCircleCopy`.
 */
typedef struct SRPacketCircle {
   /**
    * The array of packets in the circle buffer.
    */
   AVPacket* packets;

   /**
    * The size of the above array.
    */
   size_t size;

   /**
    * The index of the first packet. Starts as 0 until the packet circle becomes full in
    * which case it becomes `tail` + 1.
    */
   size_t head;

   /**
    * The index of the packet being written to. Starts as 0 and increments every rotation,
    * looping around the circle.
    */
   size_t tail;

   /**
    * A mutex used internally for synchronization.
    */
   pthread_mutex_t mutex;

   /**
    * A pointer to the current packet to write into.
    */
   AVPacket* input;
} SRPacketCircle;

/**
 * Creates a new packet circle. The size of the circle buffer will be based on the
 * program configuration.
 */
void srPacketCircleCreate(SRPacketCircle* pktCircle);

/**
 * Destroys the provided packet circle.
 */
void srPacketCircleDestroy(SRPacketCircle* pktCircle);

/**
 * Rotate the packet circle such that the current `input` is ready for copying and the
 * next packet is freed and provided as an input.
 */
void srPacketCircleRotate(SRPacketCircle* pktCircle);

/**
 * Clears all the data held in the packet circle.
 */
void srPacketCircleClear(SRPacketCircle* pktCircle);

/**
 * Copy the data from `src` into `dst`. The current `input` field of `src` will not be
 * copied. This function is safe to call even if another thread is using
 * `srPacketCircleRotate` (which changes the `input` field) on `src`. This will also
 * "normalize" the packets such that you can easily iterate `dst` from 0 to `tail`.
 */
void srPacketCircleCopy(SRPacketCircle* dst, const SRPacketCircle* src);

#endif
