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

#include "util.h"

char *rsFormat(const char *fmt, ...) {
   va_list args;
   va_start(args, fmt);
   char *value = rsFormatv(fmt, args);
   va_end(args);
   return value;
}

char *rsFormatv(const char *fmt, va_list args) {
   int ret;
   va_list copy;
   va_copy(copy, args);
   ret = vsnprintf(NULL, 0, fmt, copy);
   va_end(copy);
   if (ret < 0) {
      return NULL;
   }

   size_t size = (size_t)ret + 1;
   char *value = av_malloc(size);
   if (value != NULL) {
      vsnprintf(value, size, fmt, args);
   }
   return value;
}

void rsOptionsSet(AVDictionary **options, int *error, const char *key, const char *fmt,
                  ...) {
   va_list args;
   va_start(args, fmt);
   rsOptionsSetv(options, error, key, fmt, args);
   va_end(args);
}

void rsOptionsSetv(AVDictionary **options, int *error, const char *key, const char *fmt,
                   va_list args) {
   if (*error < 0) {
      return;
   }
   char *value = rsFormatv(fmt, args);
   if (value == NULL) {
      *error = AVERROR(ENOMEM);
   } else {
      *error = av_dict_set(options, key, value, AV_DICT_DONT_STRDUP_VAL);
   }
}

void rsOptionsDestroy(AVDictionary **options) {
   if (av_dict_count(*options) > 0) {
      const char *unused = av_dict_get(*options, "", NULL, AV_DICT_IGNORE_SUFFIX)->key;
      av_log(NULL, AV_LOG_WARNING, "Unused option: %s\n", unused);
   }
   av_dict_free(options);
}

int rsXDisplayOpen(RSXDisplay **display, const char *name) {
#ifdef RS_BUILD_X11_FOUND
   *display = XOpenDisplay(name);
   if (*display == NULL) {
      av_log(NULL, AV_LOG_ERROR, "Failed to open X11 display\n");
      return AVERROR_EXTERNAL;
   } else {
      av_log(NULL, AV_LOG_INFO, "X11 version: %i.%i\n", ProtocolVersion(*display),
             ProtocolRevision(*display));
      av_log(NULL, AV_LOG_INFO, "X11 vendor: %s v%i\n", ServerVendor(*display),
             VendorRelease(*display));
      return 0;
   }
#else

   (void)display;
   (void)name;
   av_log(NULL, AV_LOG_ERROR, "X11 was not found during compilation\n");
   return AVERROR(ENOSYS);
#endif
}
