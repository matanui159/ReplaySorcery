/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "log.h"

#define LOG_FILE(file, fmt)                                                              \
   do {                                                                                  \
      va_list args;                                                                      \
      va_start(args, fmt);                                                               \
      vfprintf(file, fmt, args);                                                         \
      va_end(args);                                                                      \
      fputc('\n', file);                                                                 \
   } while (0)

void rsLog(const char *fmt, ...) {
   LOG_FILE(stdout, fmt);
}

void rsError(const char *fmt, ...) {
   LOG_FILE(stderr, fmt);
   // TODO: include libbacktrace
   exit(EXIT_FAILURE);
}
