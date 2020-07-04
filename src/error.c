#include "error.h"
#include "sr-config.h"
#include <stdlib.h>
#include <signal.h>

#define ERROR_SIGNAL 1 // UNW_INIT_SIGNAL_FRAME
#define ERROR_NAME_SIZE 1024

#ifdef SR_CONFIG_UNWIND
#define UNW_LOCAL_ONLY
#include <libunwind.h>

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
         av_log(NULL, AV_LOG_INFO, " - %s+%"PRIXPTR"\n", name, offset);
      }
   } while ((ret = unw_step(&cursor)) > 0);
   return ret;
}
#endif

static av_noreturn void errorImpl(int error, int flags) {
   av_log(NULL, AV_LOG_FATAL, "%s\n", av_err2str(error));
#ifdef SR_CONFIG_UNWIND
   if (errorUnwind(flags) < 0) {
      av_log(NULL, AV_LOG_WARNING, "Failed to unwind stack\n");
   }
#else
   (void)flags;
   av_log(NULL, AV_LOG_WARNING, "Compile with libunwind for more info\n");
#endif
   exit(EXIT_FAILURE);
}

static void errorSignal(int signal) {
   const char* msg;
   switch (signal) {
      case SIGILL:
         msg = "Illegal program\n";
         break;
      case SIGFPE:
         msg = "Floating point error\n";
         break;
      case SIGSEGV:
         msg = "Segment violation\n";
         break;
   }
   av_log(NULL, AV_LOG_ERROR, msg);
   errorImpl(AVERROR_BUG, ERROR_SIGNAL);
}

void srErrorInit(void) {
   signal(SIGSEGV, errorSignal);
   signal(SIGILL, errorSignal);
   signal(SIGFPE, errorSignal);
}

av_noreturn void srError(int error) {
   errorImpl(error, 0);
}
