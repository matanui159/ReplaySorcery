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

#include "log.h"
#include "config.h"
#include "thread.h"
#include <backtrace.h>
#include <libavutil/avutil.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>

static struct backtrace_state *traceState = NULL;
static RSMutex silenceMutex;
static int silenceCounter;

static void logDefault(void *ctx, int level, const char *format, ...) {
   va_list args;
   va_start(args, format);
   av_log_default_callback(ctx, level, format, args);
   va_end(args);
}

static void logTraceError(void *extra, const char *message, int error) {
   (void)extra;
   logDefault(NULL, AV_LOG_WARNING, "Backtrace error: %s (%i)\n", message, error);
}

static int logTrace(void *extra, uintptr_t pc, const char *file, int line,
                    const char *func) {
   (void)pc;
   int *level = extra;
   if (file != NULL || func != NULL) {
      logDefault(NULL, *level, " - %s:%i (%s)\n", file, line, func);
   }
   return 0;
}

static void logCallback(void *ctx, int level, const char *format, va_list args) {
   av_log_default_callback(ctx, level, format, args);
   if (level <= rsConfig.traceLevel && traceState != NULL) {
      backtrace_full(traceState, 1, logTrace, logTraceError, &level);
   }
}

static void logSignal(int signal) {
   const char *error = NULL;
   switch (signal) {
   case SIGSEGV:
      error = "Segment violation";
      break;
   case SIGILL:
      error = "Illegal program";
      break;
   case SIGFPE:
      error = "Floating point error";
      break;
   }

   av_log(NULL, AV_LOG_PANIC, "%s\n", error);
   abort();
}

int rsLogInit(void) {
   int ret;
   signal(SIGSEGV, logSignal);
   signal(SIGILL, logSignal);
   signal(SIGFPE, logSignal);
   av_log_set_callback(logCallback);
   traceState = backtrace_create_state(NULL, 1, logTraceError, NULL);
   rsConfig.traceLevel = AV_LOG_ERROR;
   if ((ret = rsMutexCreate(&silenceMutex)) < 0) {
      return ret;
   }
   return 0;
}

void rsLogExit(void) {
   rsMutexDestroy(&silenceMutex);
}

void rsLogSilence(int silence) {
   rsMutexLock(&silenceMutex);
   silence += silenceCounter;
   if (silence <= 0) {
      silence = 0;
      if (silenceCounter > 0) {
         av_log_set_level(rsConfig.logLevel);
         av_log(NULL, AV_LOG_WARNING, "Logs have been unsilenced\n");
      }
   } else if (silenceCounter == 0) {
      av_log(NULL, AV_LOG_WARNING, "Silencing logs for now\n");
      av_log_set_level(AV_LOG_FATAL);
   }
   silenceCounter = silence;
   rsMutexUnlock(&silenceMutex);
}
