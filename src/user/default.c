/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "default.h"
#include "../config/config.h"
#include "../error.h"
#include "debug.h"
#include "x11.h"
#include <libavutil/avutil.h>

void rsDefaultUserCreate(RSUser* user) {
   int ret;

   // Try to create a debug user if enabled.
   if (rsConfig.enableDebug) {
      if ((ret = rsDebugUserCreate(user)) >= 0) return;
      av_log(NULL, AV_LOG_WARNING, "Failed to create debug user: %s\n", av_err2str(ret));
   }

   // Try to create an X11 user.
   if (!rsConfig.disableX11User) {
      if ((ret = rsX11UserCreate(user)) >= 0) return;
      av_log(NULL, AV_LOG_WARNING, "Failed to create X11 user: %s\n", av_err2str(ret));
   }

   rsError(AVERROR(ENOSYS));
}
