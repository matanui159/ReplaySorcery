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

#include "log.h"
#include <backtrace.h>

#define LOG_FILE(file, fmt)                                                              \
   do {                                                                                  \
      va_list args;                                                                      \
      va_start(args, fmt);                                                               \
      vfprintf(file, fmt, args);                                                         \
      va_end(args);                                                                      \
      fputc('\n', file);                                                                 \
      fflush(file);                                                                      \
   } while (0)

static void backtraceError(void *data, const char *error, int code) {
   (void)data;
   rsLog("Backtrace error: %s (%i)\n", error, code);
}

void rsLog(const char *fmt, ...) {
   LOG_FILE(stdout, fmt);
}

void rsError(const char *fmt, ...) {
   LOG_FILE(stderr, fmt);
   struct backtrace_state *backtrace =
       backtrace_create_state(NULL, false, backtraceError, NULL);
   if (backtrace != NULL) {
      backtrace_print(backtrace, 0, stderr);
   }
   exit(EXIT_FAILURE);
}
