/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "string.h"
#include "memory.h"
#include <ctype.h>

char *rsStringTrimStart(char *str) {
   while (isspace(*str)) {
      ++str;
   }
   return str;
}

char *rsStringTrimEnd(char *str) {
   size_t size = strlen(str);
   while (size > 0 && isspace(str[size - 1])) {
      str[size - 1] = '\0';
      --size;
   }
   return str;
}

char *rsStringSplit(char **str, char c) {
   if (*str == NULL) {
      return NULL;
   }
   char *ret = *str;
   char *split = strchr(*str, c);
   if (split == NULL) {
      *str = NULL;
   } else {
      *split = '\0';
      *str = split + 1;
   }
   return ret;
}
