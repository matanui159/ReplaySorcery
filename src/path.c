/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "path.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

void rsPathJoin(AVBPrint* path, const char* extra, RSPathFlags flags) {
   if (extra[0] == '/') {
      // Absolute path
      av_bprint_clear(path);
   }
   if (extra[0] == '~' && extra[1] == '/') {
      // $HOME-relative path
      rsPathJoin(path, getenv("HOME"), 0);
      extra += 2;
   }

   if (path->len > 0 && path->str[path->len - 1] != '/') {
      // The path is missing a final `/` so we have to add one.
      av_bprint_chars(path, '/', 1);
   }
   if (flags & RS_PATH_STRFTIME) {
      time_t timeNum = time(NULL);
      struct tm* timeObj = localtime(&timeNum);
      av_bprint_strftime(path, extra, timeObj);
   } else {
      av_bprint_append_data(path, extra, (unsigned)strlen(extra));
   }
}
