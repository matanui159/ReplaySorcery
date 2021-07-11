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

#ifndef RS_THREAD_H
#define RS_THREAD_H
#include "rsbuild.h"
#include <libavutil/avutil.h>
#ifdef RS_BUILD_PTHREAD_FOUND
#include <pthread.h>
#endif

typedef void *(*RSThreadFunction)(void *extra);

typedef struct RSThread {
#ifdef RS_BUILD_PTHREAD_FOUND
   pthread_t pthread;
#endif
   int created;
} RSThread;

typedef struct RSMutex {
#ifdef RS_BUILD_PTHREAD_FOUND
   pthread_mutex_t mutex;
#endif
   int created;
} RSMutex;

int rsThreadCreate(RSThread *thread, RSThreadFunction func, void *extra);
void *rsThreadDestroy(RSThread *thread);
int rsMutexCreate(RSMutex *mutex);
void rsMutexDestroy(RSMutex *mutex);
void rsMutexLock(RSMutex *mutex);
void rsMutexUnlock(RSMutex *mutex);

#endif
