/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "common.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef RS_CONFIG_LIB_UNWIND
#define UNW_LOCAL_ONLY
#include <libunwind.h>
#endif

static void logArgs(const char *fmt, va_list args) {
   va_list copy;
   va_copy(copy, args);
   int ret = vsnprintf(NULL, 0, fmt, copy);
   va_end(copy);
   if (ret < 0) {
      return;
   }

   size_t size = (size_t)ret + 1;
   char buffer[size];
   vsnprintf(buffer, size, fmt, args);
   printf("%s\n", buffer);
}

static void errorUnwind(RSErrorFlags flags) {
#ifdef RS_CONFIG_LIB_UNWIND
   unw_context_t context;
   if (unw_getcontext(&context) < 0) {
      return;
   }

   unw_cursor_t cursor;
   if (unw_init_local2(&cursor, &context, flags) < 0) {
      return;
   }

   do {
      char name[1024];
      unw_word_t offset;
      if (unw_get_proc_name(&cursor, name, sizeof(name), &offset) == 0) {
         rsLog(" - %s +%" PRIXPTR, name, offset);
      } else {
         rsLog(" - ???");
      }
   } while (unw_step(&cursor) > 0);
#else
   (void)flags;
#endif
}

static void memoryCheck(void *ptr, size_t size) {
   if (ptr == NULL) {
      rsError(0, "Out of memory when allocating %zu bytes", size);
   }
}

void rsLog(const char *fmt, ...) {
   va_list args;
   va_start(args, fmt);
   logArgs(fmt, args);
   va_end(args);
}

RS_NORETURN void rsError(RSErrorFlags flags, const char *fmt, ...) {
   va_list args;
   va_start(args, fmt);
   logArgs(fmt, args);
   va_end(args);

   errorUnwind(flags);
   exit(EXIT_FAILURE);
}

void *rsAllocate(size_t size) {
   if (size == 0) {
      return NULL;
   }
   void *ptr = malloc(size);
   memoryCheck(ptr, size);
   return ptr;
}

void rsFree(void *ptr) {
   free(ptr);
}

void *rsReallocate(void *ptr, size_t size) {
   if (size == 0) {
      return ptr;
   }
   ptr = realloc(ptr, size);
   memoryCheck(ptr, size);
   return ptr;
}
