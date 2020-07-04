#include "default.h"
#include "debug.h"
#include "../error.h"
#include <libavutil/avutil.h>

void srDefaultUserCreate(SRUser* user) {
   int ret;
   if ((ret = srDebugUserCreate(user)) >= 0) {
      return;
   }
   av_log(NULL, AV_LOG_WARNING, "Failed to create debug user: %s\n", av_err2str(ret));

   srError(AVERROR(ENOSYS));
}
