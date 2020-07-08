/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef RS_PATH_H
#define RS_PATH_H
#include <libavutil/avutil.h>
#include <libavutil/bprint.h>

/**
 * Flags to be used for when joining paths.
 */
typedef enum RSPathFlags {
   /**
    * Generate the appended path by passing it through `strftime` to create a datetime
    * relative path.
    */
   RS_PATH_STRFTIME = 1
} RSPathFlags;

/**
 * A utility function to add `extra` to the dynamically-sized `path`. Will treat paths
 * starting with `/` as absolute, expand paths starting with `~/` and otherwise make sure
 * there is a `/` between `path` and `extra`.
 */
void rsPathJoin(AVBPrint* path, const char* extra, RSPathFlags flags);

#endif
