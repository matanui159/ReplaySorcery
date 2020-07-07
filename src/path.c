/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "path.h"
#include "error.h"
#include <libavutil/avutil.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/**
 * The initial capacity of the path.
 */
#define PATH_CAPACITY 16

/**
 * Adds a formatted string to the path.
 */
static void pathPrintf(RSPath* path, const char* fmt, ...) {
   va_list args;
   va_start(args, fmt);
   int ret = vsnprintf(NULL, 0, fmt, args);
   va_end(args);
   if (ret < 0) rsError(EINVAL);

   // Make sure the capacity is large enough and reallocate if necessary.
   size_t size = path->size + (size_t)ret + 1;
   if (size > path->capacity) {
      while (size > path->capacity)
         path->capacity *= 2;
      rsCheck(av_reallocp(&path->value, path->capacity));
   }

   va_start(args, fmt);
   vsprintf(path->value + path->size, fmt, args);
   va_end(args);
   path->size = size - 1;
}

void rsPathCreate(RSPath* path, const char* str) {
   path->size = 0;
   path->capacity = PATH_CAPACITY;
   path->value = av_malloc(path->capacity);
   if (str == NULL) {
      path->value[0] = '\0';
   } else {
      rsPathAdd(path, str);
   }
}

void rsPathDestroy(RSPath* path) {
   av_freep(&path->value);
}

void rsPathAdd(RSPath* path, const char* str) {
   if (str[0] == '/') {
      // This is an absolute directory.
      path->size = 0;
   }
   if (str[0] == '~' && str[1] == '/') {
      // This is a home directory.
      rsPathAdd(path, getenv("HOME"));
      str += 2;
   }

   // `strchr` also considers `\0` part of the string and thus can be used to get the end
   // of the string.
   char* end = strchr(str, '\0') - 1;
   if (path->size == 0 || *end == '/') {
      pathPrintf(path, "%s", str);
   } else {
      pathPrintf(path, "/%s", str);
   }
}

void rsPathAddDated(RSPath* path, const char* str) {
   time_t timeNum = time(NULL);
   struct tm* timeObj = localtime(&timeNum);
   size_t timeSize = PATH_CAPACITY;
   char* timeBuf = av_malloc(timeSize);
   while (strftime(timeBuf, timeSize, str, timeObj) == 0) {
      timeSize *= 2;
      rsCheck(av_reallocp(&timeBuf, timeSize));
   }

   rsPathAdd(path, timeBuf);
   av_freep(&timeBuf);
}
