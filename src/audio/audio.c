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

#include "audio.h"
#include "../util.h"
#include "adevice.h"
#include "aencoder.h"

#ifdef RS_BUILD_PTHREAD_FOUND
static void *audioThread(void *extra) {
   int ret;
   RSAudioThread *thread = extra;
   while (thread->running) {
      if ((ret = rsDeviceNextFrame(&thread->device, thread->frame)) < 0) {
         av_log(NULL, AV_LOG_WARNING, "Failed to frame from audio device: %s\n",
                av_err2str(ret));
      } else {
         rsAudioThreadLock(thread);
         ret = rsAudioBufferAddFrame(&thread->buffer, thread->frame);
         rsAudioThreadUnlock(thread);
         if (ret < 0) {
            goto error;
         }
      }
   }

   ret = 0;
error:
   if (ret < 0) {
      av_log(NULL, AV_LOG_FATAL, "Audio thread exiting: %s\n", av_err2str(ret));
   }
   return NULL;
}
#endif

int rsAudioThreadCreate(RSAudioThread *thread) {
#ifdef RS_BUILD_PTHREAD_FOUND
   int ret;
   rsClear(thread, sizeof(RSAudioThread));
   if ((ret = rsAudioDeviceCreate(&thread->device)) < 0) {
      goto error;
   }
   if ((ret = rsAudioBufferCreate(&thread->buffer, thread->device.params)) < 0) {
      goto error;
   }

   thread->frame = av_frame_alloc();
   if (thread->frame == NULL) {
      ret = AVERROR(ENOMEM);
      goto error;
   }

   thread->mutex = av_malloc(sizeof(pthread_mutex_t));
   if (thread->mutex == NULL) {
      ret = AVERROR(ENOMEM);
      goto error;
   }
   if ((ret = pthread_mutex_init(thread->mutex, NULL)) != 0) {
      av_freep(&thread->mutex);
      ret = AVERROR(ret);
      av_log(NULL, AV_LOG_ERROR, "Failed to create mutex: %s\n", av_err2str(ret));
      goto error;
   }

   thread->running = 1;
   thread->thread = av_malloc(sizeof(pthread_t));
   if (thread->thread == NULL) {
      ret = AVERROR(ENOMEM);
      goto error;
   }
   if ((ret = pthread_create(thread->thread, NULL, audioThread, thread)) != 0) {
      av_freep(&thread->thread);
      ret = AVERROR(ret);
      av_log(NULL, AV_LOG_ERROR, "Failed to create thread: %s\n", av_err2str(ret));
      goto error;
   }

   return 0;
error:
   rsAudioThreadDestroy(thread);
   return ret;

#else
   (void)thread;
   av_log(NULL, AV_LOG_WARNING, "Pthreads was not found during compilation\n");
   av_log(NULL, AV_LOG_ERROR, "Pthreads is required for audio support\n");
   return AVERROR(ENOSYS);
#endif
}

void rsAudioThreadDestroy(RSAudioThread *thread) {
#ifdef RS_BUILD_PTHREAD_FOUND
   thread->running = 0;
   if (thread->thread != NULL) {
      pthread_join(*thread->thread, NULL);
      av_freep(&thread->thread);
   }
   if (thread->mutex != NULL) {
      pthread_mutex_destroy(thread->mutex);
      av_freep(thread->mutex);
   }
   av_frame_free(&thread->frame);
   rsAudioBufferDestroy(&thread->buffer);
   rsDeviceDestroy(&thread->device);

#else
   (void)thread;
#endif
}

void rsAudioThreadLock(RSAudioThread *thread) {
#ifdef RS_BUILD_PTHREAD_FOUND
   pthread_mutex_lock(thread->mutex);

#else
   (void)thread;
#endif
}

void rsAudioThreadUnlock(RSAudioThread *thread) {
#ifdef RS_BUILD_PTHREAD_FOUND
   pthread_mutex_unlock(thread->mutex);

#else
   (void)thread;
#endif
}
