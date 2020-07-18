/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef RS_UTIL_STRING_H
#define RS_UTIL_STRING_H
#include "../std.h"

char *rsStringTrimStart(char *str);
char *rsStringTrimEnd(char *str);
char *rsStringSplit(char **str, char c);

#endif
