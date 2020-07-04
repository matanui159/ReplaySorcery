/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "video.h"
#include "../error.h"
#include <libavutil/avutil.h>
#include <libavutil/dict.h>

void rsVideoInputCreate(RSInput* input) {
   int ret;
   AVDictionary* options = NULL;

   // Try to create an X11 input.
   // TODO: set these options from program configuration when implemented.
   av_dict_set(&options, "video_size", "1920x1080", 0);
   av_dict_set_int(&options, "framerate", 30, 0);
   if ((ret = rsInputCreate(input, "x11grab", ":0.0", &options)) >= 0) {
      return;
   }
   av_log(NULL, AV_LOG_WARNING, "Failed to create X11 input: %s\n", av_err2str(ret));

   // All the options failed, fail with `ENOSYS` (not implemented).
   rsError(AVERROR(ENOSYS));
}
