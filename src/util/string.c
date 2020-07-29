/*
 * Copyright (C) 2020  Joshua Minter
 *
 * This file is part of ReplaySorcery.
 *
 * ReplaySorcery is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ReplaySorcery is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ReplaySorcery.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "string.h"
#include "memory.h"
#include <ctype.h>

char *rsStringClone(const char *str) {
   size_t size = strlen(str) + 1;
   char *clone = rsMemoryCreate(size);
   memcpy(clone, str, size);
   return clone;
}

char *rsStringTrim(char *str) {
   return rsStringTrimEnd(rsStringTrimStart(str));
}

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
