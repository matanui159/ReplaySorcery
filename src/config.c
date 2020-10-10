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
#include "rsbuild.h"
#include <libavformat/avio.h>
#include <libavutil/avstring.h>
#include <libavutil/bprint.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>

#define CONFIG_CONST(name, value, group)                                                 \
   { #name, NULL, 0, AV_OPT_TYPE_CONST, {.i64 = (value) }, 0, 0, 0, #group }
#define CONFIG_INT(name, def, min, max, group)                                           \
   {                                                                                     \
#name, NULL, offsetof(RSConfig, name), AV_OPT_TYPE_INT,                            \
          {.i64 = (def) }, (min), (max), 0, #group                                       \
   }

static const AVOption configOptions[] = {
    CONFIG_CONST(auto, RS_CONFIG_AUTO, auto),
    CONFIG_INT(logLevel, AV_LOG_INFO, AV_LOG_QUIET, AV_LOG_TRACE, logLevel),
    CONFIG_INT(traceLevel, AV_LOG_ERROR, AV_LOG_QUIET, AV_LOG_TRACE, logLevel),
    CONFIG_CONST(quiet, AV_LOG_QUIET, logLevel),
    CONFIG_CONST(panic, AV_LOG_PANIC, logLevel),
    CONFIG_CONST(fatal, AV_LOG_FATAL, logLevel),
    CONFIG_CONST(error, AV_LOG_ERROR, logLevel),
    CONFIG_CONST(warning, AV_LOG_WARNING, logLevel),
    CONFIG_CONST(info, AV_LOG_INFO, logLevel),
    CONFIG_CONST(verbose, AV_LOG_VERBOSE, logLevel),
    CONFIG_CONST(debug, AV_LOG_DEBUG, logLevel),
    CONFIG_CONST(trace, AV_LOG_TRACE, logLevel),
    CONFIG_INT(videoX, 0, 0, INT_MAX, NULL),
    CONFIG_INT(videoY, 0, 0, INT_MAX, NULL),
    CONFIG_INT(videoWidth, RS_CONFIG_AUTO, RS_CONFIG_AUTO, INT_MAX, auto),
    CONFIG_INT(videoHeight, RS_CONFIG_AUTO, RS_CONFIG_AUTO, INT_MAX, auto),
    CONFIG_INT(videoFramerate, 30, 0, INT_MAX, NULL),
    CONFIG_INT(videoInput, RS_CONFIG_AUTO, RS_CONFIG_AUTO, RS_CONFIG_VIDEO_X11,
               videoInput),
    CONFIG_CONST(auto, RS_CONFIG_AUTO, videoInput),
    CONFIG_CONST(x11, RS_CONFIG_VIDEO_X11, videoInput),
    CONFIG_INT(videoEncoder, RS_CONFIG_AUTO, RS_CONFIG_AUTO, RS_CONFIG_VIDEO_X264,
               videoEncoder),
    CONFIG_CONST(auto, RS_CONFIG_AUTO, videoEncoder),
    CONFIG_CONST(x264, RS_CONFIG_VIDEO_X264, videoEncoder),
    CONFIG_INT(videoProfile, FF_PROFILE_H264_BASELINE, FF_PROFILE_H264_BASELINE, FF_PROFILE_H264_HIGH, videoProfile),
    CONFIG_CONST(baseline, FF_PROFILE_H264_BASELINE, videoProfile),
    CONFIG_CONST(main, FF_PROFILE_H264_MAIN, videoProfile),
    CONFIG_CONST(high, FF_PROFILE_H264_HIGH, videoProfile),
    CONFIG_INT(videoPreset, RS_CONFIG_PRESET_FAST, RS_CONFIG_PRESET_FAST, RS_CONFIG_PRESET_SLOW, videoPreset),
    CONFIG_CONST(fast, RS_CONFIG_PRESET_FAST, videoPreset),
    CONFIG_CONST(medium, RS_CONFIG_PRESET_MEDIUM, videoPreset),
    CONFIG_CONST(slow, RS_CONFIG_PRESET_SLOW, videoPreset),
    CONFIG_INT(videoQuality, 30, RS_CONFIG_AUTO, 51, auto),
    CONFIG_INT(videoThreads, 1, RS_CONFIG_AUTO, INT_MAX, auto),
    CONFIG_INT(videoScaler, SWS_FAST_BILINEAR, SWS_FAST_BILINEAR, SWS_SPLINE,
               videoScaler),
    CONFIG_CONST(fast, SWS_FAST_BILINEAR, videoScaler),
    CONFIG_CONST(bilinear, SWS_BILINEAR, videoScaler),
    CONFIG_CONST(bicubic, SWS_BICUBIC, videoScaler),
    CONFIG_CONST(point, SWS_POINT, videoScaler),
    CONFIG_CONST(area, SWS_AREA, videoScaler),
    CONFIG_CONST(bicublin, SWS_BICUBLIN, videoScaler),
    CONFIG_CONST(gauss, SWS_GAUSS, videoScaler),
    CONFIG_CONST(sinc, SWS_SINC, videoScaler),
    CONFIG_CONST(lanczos, SWS_LANCZOS, videoScaler),
    CONFIG_CONST(spline, SWS_SPLINE, videoScaler),
    {NULL}};

static const AVClass configClass = {
    .class_name = "ReplaySorcery",
    .option = configOptions,
    .item_name = av_default_item_name,
};

RSConfig rsConfig = {.avClass = &configClass};

static char *configTrim(char *str) {
   while (av_isspace(*str)) {
      ++str;
   }
   char *end = strchr(str, 0) - 1;
   while (av_isspace(*end)) {
      --end;
   }
   end[1] = 0;
   return str;
}

int rsConfigInit(void) {
   int ret;
   av_opt_set_defaults(&rsConfig);
   AVIOContext *file;
   if ((ret = avio_open(&file, RS_BUILD_CONFIG_FILE, AVIO_FLAG_READ)) < 0) {
      av_log(NULL, AV_LOG_WARNING, "Failed to open config file: %s\n", av_err2str(ret));
      return 0;
   }

   AVBPrint buffer;
   char *contents = NULL;
   av_bprint_init(&buffer, 0, AV_BPRINT_SIZE_UNLIMITED);
   if ((ret = avio_read_to_bprint(file, &buffer, INT_MAX)) < 0) {
      av_log(NULL, AV_LOG_ERROR, "Failed to read config file: %s\n", av_err2str(ret));
      goto error;
   }
   if ((ret = av_bprint_finalize(&buffer, &contents)) < 0) {
      goto error;
   }

   const char *delim = "\n\r";
   char *state;
   for (char *line = av_strtok(contents, delim, &state); line != NULL;
        line = av_strtok(NULL, delim, &state)) {
      // Remove comments
      char *hash = strchr(line, '#');
      if (hash != NULL) {
         *hash = 0;
      }

      // Ignore empty lines
      line = configTrim(line);
      if (*line == 0) {
         continue;
      }

      // Get key and valye
      char *eq = strchr(line, '=');
      if (eq == NULL) {
         av_log(NULL, AV_LOG_ERROR,
                "Config file format: key = value [# optional comment]\n");
         goto error;
      }

      *eq = 0;
      char *key = configTrim(line);
      char *value = configTrim(eq + 1);
      if ((ret = av_opt_set(&rsConfig, key, value, 0)) < 0) {
         av_log(NULL, AV_LOG_ERROR, "Failed to set '%s' to '%s': %s\n", key, value,
                av_err2str(ret));
         goto error;
      }
      av_log_set_level(rsConfig.logLevel);
   }

   ret = 0;
error:
   av_freep(&contents);
   av_bprint_finalize(&buffer, NULL);
   avio_closep(&file);
   return ret;
}
