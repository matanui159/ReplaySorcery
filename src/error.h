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
