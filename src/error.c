/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "error.h"
#include "rs-config.h"
#include <signal.h>
#include <stdlib.h>

/**
 * Same as the `UNW_INIT_SIGNAL_FRAME` flag but always defined even if libunwind is not
 * supported.
 */
#define ERROR_SIGNAL 1

/**
 * The size for the name buffer when getting function names.
 */
#define ERROR_NAME_SIZE 1024

#ifdef RS_CONFIG_UNWIND
#define UNW_LOCAL_ONLY
#include <libunwind.h>

/**
 * Logs an libunwind-generated stack trace. Returns 0 on success or a negative code on
 * failure. Note that negative codes are not libav error codes and should not be passed to
 * `rsError`.
 */
static int errorUnwind(int flags) {
   int ret;
   unw_context_t context;
   if ((ret = unw_getcontext(&context)) < 0) return ret;
   unw_cursor_t cursor;
   if ((ret = unw_init_local2(&cursor, &context, flags)) < 0) return ret;
   do {
      char name[ERROR_NAME_SIZE];
      unw_word_t offset;
      if (unw_get_proc_name(&cursor, name, ERROR_NAME_SIZE, &offset) < 0) {
         av_log(NULL, AV_LOG_INFO, " - ???\n");
      } else {
         av_log(NULL, AV_LOG_INFO, " - %s+%" PRIXPTR "\n", name, offset);
      }
   } while ((ret = unw_step(&cursor)) > 0);
   return ret;
}
#endif

/**
 * Internal implementation of `rsError`. Allows passing flags for generating the stack
 * trace.
 */
static av_noreturn void errorImpl(int error, int flags) {
   av_log(NULL, AV_LOG_FATAL, "%s\n", av_err2str(error));
#ifdef RS_CONFIG_UNWIND
   if (errorUnwind(flags) < 0) {
      av_log(NULL, AV_LOG_WARNING, "Failed to unwind stack\n");
   }
#else
   (void)flags;
   av_log(NULL, AV_LOG_WARNING, "Compile with libunwind for more info\n");
#endif
   exit(EXIT_FAILURE);
}

/**
 * Signal handler for segment fault, invalid programs or floating-point errors.
 */
static void errorSignal(int signal) {
   switch (signal) {
   case SIGSEGV:
      av_log(NULL, AV_LOG_ERROR, "Segment violation\n");
      break;
   case SIGILL:
      av_log(NULL, AV_LOG_ERROR, "Illegal program\n");
      break;
   case SIGFPE:
      av_log(NULL, AV_LOG_ERROR, "Floating point error\n");
      break;
   }
   // We use `ERROR_SIGNAL` here since we are in a signal handler.
   errorImpl(AVERROR_BUG, ERROR_SIGNAL);
}

void rsErrorInit(void) {
   signal(SIGSEGV, errorSignal);
   signal(SIGILL, errorSignal);
   signal(SIGFPE, errorSignal);
}

av_noreturn void rsError(int error) {
   errorImpl(error, 0);
}
