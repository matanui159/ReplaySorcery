/*
 * Copyright (C) 2020-2021  Joshua Minter
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
#include "config.h"
#include "rsbuild.h"
#ifdef RS_BUILD_POSIX_IO_FOUND
#include <sys/stat.h>
#endif

static int directoryCreate(char *path) {
#ifdef RS_BUILD_POSIX_IO_FOUND
   int ret;
   char *slash = strrchr(path, '/');
   if (slash == NULL) {
      return 0;
   }

   *slash = '\0';
   if ((ret = mkdir(path, 0777)) < 0) {
      ret = errno;
   }
   if (ret == ENOENT) {
      if ((ret = directoryCreate(path)) < 0) {
         goto error;
      }
      if ((ret = mkdir(path, 0777)) < 0) {
         ret = errno;
      }
   }
   if (ret != 0 && ret != EEXIST) {
      ret = AVERROR(ret);
      av_log(NULL, AV_LOG_ERROR, "Failed to create directory: %s\n", av_err2str(ret));
      goto error;
   }

   ret = 0;
error:
   *slash = '/';
   return ret;

#else
   (void)path;
   av_log(NULL, AV_LOG_WARNING,
          "Failed to create directory: Posix I/O was not found during compilation\n");
   return 0;
#endif
}

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

AVCodecParameters *rsParamsClone(const AVCodecParameters *params) {
   AVCodecParameters *clone = avcodec_parameters_alloc();
   if (clone == NULL) {
      goto error;
   }
   if (avcodec_parameters_copy(clone, params) < 0) {
      goto error;
   }

   return clone;
error:
   avcodec_parameters_free(&clone);
   return NULL;
}

void rsScaleSize(int *width, int *height) {
   int scaleWidth = *width;
   int scaleHeight = *height;
   if (rsConfig.scaleWidth != RS_CONFIG_AUTO) {
      scaleWidth = rsConfig.scaleWidth;
      if (rsConfig.scaleHeight == RS_CONFIG_AUTO) {
         scaleHeight = (int)av_rescale(scaleWidth, *height, *width);
      }
   }
   if (rsConfig.scaleHeight != RS_CONFIG_AUTO) {
      scaleHeight = rsConfig.scaleHeight;
      if (rsConfig.scaleWidth == RS_CONFIG_AUTO) {
         scaleWidth = (int)av_rescale(scaleHeight, *width, *height);
      }
   }
   *width = (scaleWidth >> 1) << 1;
   *height = (scaleHeight >> 1) << 1;
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

int rsDirectoryCreate(const char *path) {
   char *dup = av_strdup(path);
   if (dup == NULL) {
      return AVERROR(ENOMEM);
   }
   int ret = directoryCreate(dup);
   av_freep(&dup);
   return ret;
}
