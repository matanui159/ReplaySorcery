/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef SR_ERROR_H
#define SR_ERROR_H
#include <libavutil/avutil.h>

/**
 * Initializes the error utilities.
 */
void srErrorInit(void);

/**
 * Logs the provided error code, attempts to log a stack trace and exits with
 * `EXIT_FAILURE`.
 */
av_noreturn void srError(int error);

/**
 * Checks the provided return code and if it indicates an error, crashes with that error.
 * Otherwise, returns the code unmodified.
 */
static inline int srCheck(int ret) {
   if (ret < 0) srError(ret);
   return ret;
}

/**
 * Similar to `srCheck` but designed for POSIX return codes.
 */
static inline int srCheckPosix(int ret) {
   if (ret != 0) srError(AVERROR(ret));
   return ret;
}

#endif
