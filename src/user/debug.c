/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "debug.h"
#include "../main.h"
#include <libavutil/avutil.h>
#include <stdio.h>

static void debugUserWait(RSUser* user) {
   (void)user;
   av_log(NULL, AV_LOG_INFO, "Press enter to save\n");
   int c;
   while ((c = getchar()) != '\n') {
      if (c == EOF) rsMainExit();
   }
}

int rsDebugUserCreate(RSUser* user) {
   user->destroy = NULL;
   user->wait = debugUserWait;
   return 0;
}
