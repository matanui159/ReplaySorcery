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
