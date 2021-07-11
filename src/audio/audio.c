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

#include "audio.h"
#include "../log.h"
#include "../util.h"
#include "adevice.h"
#include "aencoder.h"

static void *audioThread(void *extra) {
   int ret;
   RSAudioThread *thread = extra;
   int silence = 0;
   while (thread->running) {
      if ((ret = rsDeviceNextFrame(&thread->device, thread->frame)) < 0) {
         av_log(NULL, AV_LOG_WARNING, "Failed to get frame from audio device: %s\n",
                av_err2str(ret));
         if (!silence) {
            rsLogSilence(1);
            silence = 1;
         }
      } else {
         if (silence) {
            rsLogSilence(-1);
            silence = 0;
         }
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

int rsAudioThreadCreate(RSAudioThread *thread) {
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
   if ((ret = rsMutexCreate(&thread->mutex)) < 0) {
      av_log(NULL, AV_LOG_ERROR, "Mutexes is required for audio support: %s\n",
             av_err2str(ret));
      return ret;
   }

   thread->running = 1;
   if ((ret = rsThreadCreate(&thread->thread, audioThread, thread)) < 0) {
      av_log(NULL, AV_LOG_ERROR, "Threads is required for audio support: %s\n",
             av_err2str(ret));
      goto error;
   }

   return 0;
error:
   rsAudioThreadDestroy(thread);
   return ret;
}

void rsAudioThreadDestroy(RSAudioThread *thread) {
   thread->running = 0;
   rsThreadDestroy(&thread->thread);
   rsMutexDestroy(&thread->mutex);
   av_frame_free(&thread->frame);
   rsAudioBufferDestroy(&thread->buffer);
   rsDeviceDestroy(&thread->device);
}

void rsAudioThreadLock(RSAudioThread *thread) {
   rsMutexLock(&thread->mutex);
}

void rsAudioThreadUnlock(RSAudioThread *thread) {
   rsMutexUnlock(&thread->mutex);
}
