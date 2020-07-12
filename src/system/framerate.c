/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "framerate.h"
#include <time.h>

#define TIME_NANO_PER_SEC 1000000000L

void rsFramerateSleep(struct timespec *prevTime, int framerate) {
   prevTime->tv_nsec += TIME_NANO_PER_SEC / framerate;
   if (prevTime->tv_nsec > TIME_NANO_PER_SEC) {
      prevTime->tv_nsec -= TIME_NANO_PER_SEC;
      ++prevTime->tv_sec;
   }
   clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, prevTime, NULL);
   clock_gettime(CLOCK_MONOTONIC, prevTime);
}
