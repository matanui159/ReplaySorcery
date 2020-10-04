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

#include "config.h"
#include <libavutil/opt.h>

#define CONFIG_CONST(name, value, group)                                                 \
   { (name), NULL, 0, AV_OPT_TYPE_CONST, {.i64 = (value)}, 0, 0, 0, (group) }
#define CONFIG_INT(name, field, def, min, max, group)                                    \
   {                                                                                     \
      (name), NULL, offsetof(RSConfig, field), AV_OPT_TYPE_INT, {.i64 = (def)}, (min),   \
          (max), 0, (group)                                                              \
   }

static const AVOption configOptions[] = {
    CONFIG_CONST("auto", RS_CONFIG_AUTO, "auto"),
    CONFIG_INT("log-level", logLevel, AV_LOG_INFO, AV_LOG_QUIET, AV_LOG_TRACE,
               "logLevel"),
    CONFIG_INT("l", logLevel, AV_LOG_INFO, AV_LOG_QUIET, AV_LOG_TRACE, "logLevel"),
    CONFIG_CONST("quiet", AV_LOG_QUIET, "logLevel"),
    CONFIG_CONST("panic", AV_LOG_PANIC, "logLevel"),
    CONFIG_CONST("fatal", AV_LOG_FATAL, "logLevel"),
    CONFIG_CONST("error", AV_LOG_ERROR, "logLevel"),
    CONFIG_CONST("warning", AV_LOG_WARNING, "logLevel"),
    CONFIG_CONST("info", AV_LOG_INFO, "logLevel"),
    CONFIG_CONST("verbose", AV_LOG_VERBOSE, "logLevel"),
    CONFIG_CONST("debug", AV_LOG_DEBUG, "logLevel"),
    CONFIG_CONST("trace", AV_LOG_TRACE, "logLevel"),
    CONFIG_INT("video-x", videoX, 0, 0, INT_MAX, NULL),
    CONFIG_INT("x", videoX, 0, 0, INT_MAX, NULL),
    CONFIG_INT("video-y", videoY, 0, 0, INT_MAX, NULL),
    CONFIG_INT("y", videoY, 0, 0, INT_MAX, NULL),
    CONFIG_INT("video-width", videoWidth, RS_CONFIG_AUTO, RS_CONFIG_AUTO, INT_MAX,
               "auto"),
    CONFIG_INT("width", videoWidth, RS_CONFIG_AUTO, RS_CONFIG_AUTO, INT_MAX, "auto"),
    CONFIG_INT("w", videoWidth, RS_CONFIG_AUTO, RS_CONFIG_AUTO, INT_MAX, "auto"),
    CONFIG_INT("video-height", videoHeight, RS_CONFIG_AUTO, RS_CONFIG_AUTO, INT_MAX,
               "auto"),
    CONFIG_INT("height", videoHeight, RS_CONFIG_AUTO, RS_CONFIG_AUTO, INT_MAX, "auto"),
    CONFIG_INT("h", videoHeight, RS_CONFIG_AUTO, RS_CONFIG_AUTO, INT_MAX, "auto"),
    CONFIG_INT("video-framerate", videoFramerate, 30, 0, INT_MAX, NULL),
    CONFIG_INT("framerate", videoFramerate, 30, 0, INT_MAX, NULL),
    CONFIG_INT("fr", videoFramerate, 30, 0, INT_MAX, NULL),
    CONFIG_INT("video-input", videoInput, RS_CONFIG_AUTO, RS_CONFIG_AUTO,
               RS_CONFIG_VIDEO_X11, "videoInput"),
    CONFIG_INT("vi", videoInput, RS_CONFIG_AUTO, RS_CONFIG_AUTO, RS_CONFIG_VIDEO_X11,
               "videoInput"),
    CONFIG_CONST("auto", RS_CONFIG_AUTO, "videoInput"),
    CONFIG_CONST("x11", RS_CONFIG_VIDEO_X11, "videoInput"),
    {NULL}};

static const AVClass configClass = {
    .class_name = "ReplaySorcery",
    .option = configOptions,
    .item_name = av_default_item_name,
};

RSConfig rsConfig = {.avClass = &configClass};

int rsConfigInit(int argc, char **argv) {
   int ret;
   av_opt_set_defaults(&rsConfig);
   for (int i = 1; i < argc; i += 2) {
      if (argv[i][0] != '-' || i == argc - 1) {
         av_log(NULL, AV_LOG_ERROR, "Argument format: -[option] [value]\n");
         return AVERROR_INVALIDDATA;
      }

      char *key = argv[i] + 1;
      if (key[0] == '-') {
         key += 1;
      }
      char *value = argv[i + 1];
      if ((ret = av_opt_set(&rsConfig, key, value, 0)) < 0) {
         av_log(NULL, AV_LOG_ERROR, "Failed to set '%s' to '%s': %s\n", key, value,
                av_err2str(ret));
         return ret;
      }
      av_log_set_level(rsConfig.logLevel);
   }
   return 0;
}
