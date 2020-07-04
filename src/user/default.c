/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "default.h"
#include "debug.h"
#include "../error.h"
#include <libavutil/avutil.h>

void rsDefaultUserCreate(RSUser* user) {
   int ret;
   if ((ret = rsDebugUserCreate(user)) >= 0) {
      return;
   }
   av_log(NULL, AV_LOG_WARNING, "Failed to create debug user: %s\n", av_err2str(ret));

   rsError(AVERROR(ENOSYS));
}
