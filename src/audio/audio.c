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
#include "adevice.h"
#include "aencoder.h"

#ifdef RS_BUILD_PTHREAD_FOUND
static void *audioThread(void *extra) {
   int ret;
   RSAudioThread *thread = extra;
   while (thread->running) {
      // if ((ret = rsStreamUpdate(thread->stream)) < 0) {
      //    goto error;
      // }
   }

   ret = 0;
error:
   if (ret < 0) {
      av_log(NULL, AV_LOG_PANIC, "Audio thread exiting: %s\n", av_err2str(ret));
   }
   return NULL;
}
#endif

int rsAudioThreadCreate(RSAudioThread **thread) {
#ifdef RS_BUILD_PTHREAD_FOUND
   int ret;
   RSAudioThread *thrd = av_mallocz(sizeof(RSAudioThread));
   *thread = thrd;
   if (thrd == NULL) {
      ret = AVERROR(ENOMEM);
      goto error;
   }
   if ((ret = rsAudioDeviceCreate(&thrd->device)) < 0) {
      goto error;
   }
   if ((ret = rsAudioEncoderCreate(&thrd->encoder, thrd->device.params)) < 0) {
      goto error;
   }
   // if ((ret = rsBufferCreate(&thrd->buffer, 0)) < 0) {
   //    goto error;
   // }

   thrd->running = 1;
   if ((ret = pthread_create(&thrd->thread, NULL, audioThread, thrd)) != 0) {
      ret = AVERROR(ret);
      av_log(NULL, AV_LOG_ERROR, "Failed to create pthread: %s\n", av_err2str(ret));
      goto error;
   }
   if (thrd->thread == 0) {
      av_log(NULL, AV_LOG_ERROR, "Thread ID of 0 not supported\n");
      ret = AVERROR_BUG;
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

void rsAudioThreadDestroy(RSAudioThread **thread) {
#ifdef RS_BUILD_PTHREAD_FOUND
   RSAudioThread *thrd = *thread;
   if (thrd != NULL) {
      thrd->running = 0;
      if (thrd->thread != 0) {
         pthread_join(thrd->thread, NULL);
      }
      rsBufferDestroy(&thrd->buffer);
      rsEncoderDestroy(&thrd->encoder);
      rsDeviceDestroy(&thrd->device);
      av_freep(thread);
   }

#else
   (void)thread;
#endif
}
