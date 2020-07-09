/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "video.h"
#include "../config/config.h"
#include "../error.h"
#include <libavutil/avstring.h>
#include <libavutil/avutil.h>
#include <libavutil/dict.h>

void rsVideoInputCreate(RSInput* input) {
   int ret;
   AVDictionary* options = NULL;

   // Try to create an X11 input.
   if (!rsConfig.disableX11Input) {
      char* videoSize = av_asprintf("%ix%i", rsConfig.inputWidth, rsConfig.inputHeight);
      av_dict_set(&options, "video_size", videoSize, AV_DICT_DONT_STRDUP_VAL);
      av_dict_set_int(&options, "grab_x", rsConfig.inputX, 0);
      av_dict_set_int(&options, "grab_y", rsConfig.inputY, 0);
      av_dict_set_int(&options, "framerate", rsConfig.inputFramerate, 0);
      if ((ret = rsInputCreate(input, "x11grab", rsConfig.inputDisplay, &options)) >= 0)
         return;
      av_log(NULL, AV_LOG_WARNING, "Failed to create X11 input: %s\n", av_err2str(ret));
   }

   // All the options failed, fail with `ENOSYS` (not implemented).
   rsError(AVERROR(ENOSYS));
}
