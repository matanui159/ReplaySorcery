/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef RS_ERROR_H
#define RS_ERROR_H
#include <errno.h>
#include <libavutil/avutil.h>

/**
 * Initializes the error utilities.
 */
void rsErrorInit(void);

/**
 * Logs the provided error code, attempts to log a stack trace and exits with
 * `EXIT_FAILURE`.
 */
av_noreturn void rsError(int error);

/**
 * Checks the provided return code and if it indicates an error, crashes with that error.
 * Otherwise, returns the code unmodified.
 */
static inline int rsCheck(int ret) {
   if (ret < 0)
      rsError(ret);
   return ret;
}

/**
 * Similar to `rsCheck` but designed for POSIX return codes.
 */
static inline int rsCheckPosix(int ret) {
   if (ret != 0)
      rsError(AVERROR(ret));
   return ret;
}

/**
 * Similar to `rsCheck` but designed for functions that set `errno`.
 */
static inline int rsCheckErrno(int ret) {
   if (ret < 0)
      rsError(AVERROR(errno));
   return ret;
}

#endif
