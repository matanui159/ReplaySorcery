/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef SR_ERROR_H
#define SR_ERROR_H
#include <libavutil/avutil.h>

void srErrorInit(void);
av_noreturn void srError(int error);

static inline int srCheck(int ret) {
   if (ret < 0) srError(ret);
   return ret;
}

static inline int srCheckPosix(int ret) {
   if (ret != 0) srError(AVERROR(ret));
   return ret;
}

#endif
