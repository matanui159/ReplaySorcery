/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "debug.h"
#include <stdio.h>
#include <libavutil/avutil.h>

static void debugUserWait(SRUser* user) {
   (void)user;
   av_log(NULL, AV_LOG_INFO, "Press enter to save\n");
   getchar();
}

int srDebugUserCreate(SRUser* user) {
   user->destroy = NULL;
   user->wait = debugUserWait;
   return 0;
}
