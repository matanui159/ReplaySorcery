/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef RS_UTIL_PATH_H
#define RS_UTIL_PATH_H
#include "../std.h"
#include "buffer.h"

void rsPathClear(RSBuffer *path);
void rsPathAppend(RSBuffer *path, const char *value);
void rsPathAppendDated(RSBuffer *path, const char* value);

#endif
