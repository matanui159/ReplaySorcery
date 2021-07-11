/*
 * Copyright (C) 2021  Joshua Minter
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

#include "thread.h"
#include "util.h"

int rsThreadCreate(RSThread *thread, RSThreadFunction func, void *extra) {
#ifdef RS_BUILD_PTHREAD_FOUND
   int ret;
   rsClear(thread, sizeof(RSThread));
   if ((ret = pthread_create(&thread->pthread, NULL, func, extra)) != 0) {
      ret = AVERROR(ret);
      av_log(NULL, AV_LOG_ERROR, "Failed to create PThread: %s\n", av_err2str(ret));
      return ret;
   }
   thread->created = 1;
   return 0;

#else
   (void)thread;
   (void)func;
   (void)extra;
   av_log(NULL, AV_LOG_ERROR,
          "A supported threads backend was not found during compilation\n");
   return AVERROR(ENOSYS);
#endif
}

void *rsThreadDestroy(RSThread *thread) {
   void *result = NULL;
#ifdef RS_BUILD_PTHREAD_FOUND
   if (thread->created) {
      pthread_join(thread->pthread, &result);
      thread->created = 0;
   }

#else
   (void)thread;
#endif
   return result;
}

int rsMutexCreate(RSMutex *mutex) {
#ifdef RS_BUILD_PTHREAD_FOUND
   int ret;
   rsClear(mutex, sizeof(RSMutex));
   if ((ret = pthread_mutex_init(&mutex->mutex, NULL)) != 0) {
      ret = AVERROR(ret);
      av_log(NULL, AV_LOG_ERROR, "Failed to create PThread mutex: %s\n", av_err2str(ret));
      return ret;
   }
   mutex->created = 1;

#else
   (void)mutex;
#endif
   return 0;
}

void rsMutexDestroy(RSMutex *mutex) {
#ifdef RS_BUILD_PTHREAD_FOUND
   if (mutex->created) {
      pthread_mutex_destroy(&mutex->mutex);
      mutex->created = 0;
   }

#else
   (void)mutex;
#endif
}

void rsMutexLock(RSMutex *mutex) {
#ifdef RS_BUILD_PTHREAD_FOUND
   int ret;
   if ((ret = pthread_mutex_lock(&mutex->mutex)) != 0) {
      ret = AVERROR(ret);
      av_log(NULL, AV_LOG_ERROR, "Failed to lock mutex: %s\n", av_err2str(ret));
   }

#else
   (void)mutex;
#endif
}

void rsMutexUnlock(RSMutex *mutex) {
#ifdef RS_BUILD_PTHREAD_FOUND
   int ret;
   if ((ret = pthread_mutex_unlock(&mutex->mutex)) != 0) {
      ret = AVERROR(ret);
      av_log(NULL, AV_LOG_ERROR, "Failed to unlock mutex: %s\n", av_err2str(ret));
   }

#else
   (void)mutex;
#endif
}
