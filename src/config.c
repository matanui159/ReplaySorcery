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

#include "config.h"
#include "rsbuild.h"
#include "util.h"
#include <libavcodec/avcodec.h>
#include <libavformat/avio.h>
#include <libavutil/avstring.h>
#include <libavutil/bprint.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>

#define CONFIG_CONST(name, value, group)                                                 \
   { #name, NULL, 0, AV_OPT_TYPE_CONST, {.i64 = value }, 0, 0, 0, #group }
#define CONFIG_STRING(name, def)                                                         \
   {                                                                                     \
#name, NULL, offsetof(RSConfig, name), AV_OPT_TYPE_STRING,                         \
          {.str = def }, 0, 0, 0, NULL                                                   \
   }
#define CONFIG_INT(name, def, min, max, group)                                           \
   {                                                                                     \
#name, NULL, offsetof(RSConfig, name), AV_OPT_TYPE_INT,                            \
          {.i64 = def }, min, max, 0, #group                                             \
   }
#define CONFIG_INT64(name, def, min, max, group)                                         \
   {                                                                                     \
#name, NULL, offsetof(RSConfig, name), AV_OPT_TYPE_INT64,                          \
          {.i64 = def }, min, max, 0, #group                                             \
   }
#define CONFIG_FLAGS(name, def, group)                                                   \
   {                                                                                     \
#name, NULL, offsetof(RSConfig, name), AV_OPT_TYPE_FLAGS,                          \
          {.i64 = def }, 0, INT_MAX, 0, #group                                           \
   }

// Remember to update replay-sorcery.conf
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
    CONFIG_INT(recordSeconds, 30, 1, INT_MAX, ),
    CONFIG_INT(videoInput, RS_CONFIG_AUTO, RS_CONFIG_DEVICE_HWACCEL,
               RS_CONFIG_DEVICE_KMS_SERVICE, videoInput),
    CONFIG_CONST(hwaccel, RS_CONFIG_DEVICE_HWACCEL, videoInput),
    CONFIG_CONST(auto, RS_CONFIG_AUTO, videoInput),
    CONFIG_CONST(x11, RS_CONFIG_DEVICE_X11, videoInput),
    CONFIG_CONST(kms, RS_CONFIG_DEVICE_KMS, videoInput),
    CONFIG_CONST(kms_service, RS_CONFIG_DEVICE_KMS_SERVICE, videoInput),
    CONFIG_STRING(videoDevice, "auto"),
    CONFIG_INT(videoX, 0, 0, INT_MAX, NULL),
    CONFIG_INT(videoY, 0, 0, INT_MAX, NULL),
    CONFIG_INT(videoWidth, RS_CONFIG_AUTO, RS_CONFIG_AUTO, INT_MAX, auto),
    CONFIG_INT(videoHeight, RS_CONFIG_AUTO, RS_CONFIG_AUTO, INT_MAX, auto),
    CONFIG_INT(videoFramerate, 30, 1, INT_MAX, NULL),
    CONFIG_INT(videoEncoder, RS_CONFIG_AUTO, RS_CONFIG_ENCODER_HEVC,
               RS_CONFIG_ENCODER_VAAPI_HEVC, videoEncoder),
    CONFIG_CONST(hevc, RS_CONFIG_ENCODER_HEVC, videoEncoder),
    CONFIG_CONST(auto, RS_CONFIG_AUTO, videoEncoder),
    CONFIG_CONST(x264, RS_CONFIG_ENCODER_X264, videoEncoder),
    CONFIG_CONST(openh264, RS_CONFIG_ENCODER_OPENH264, videoEncoder),
    CONFIG_CONST(x265, RS_CONFIG_ENCODER_X265, videoEncoder),
    CONFIG_CONST(vaapi_h264, RS_CONFIG_ENCODER_VAAPI_H264, videoEncoder),
    CONFIG_CONST(vaapi_hevc, RS_CONFIG_ENCODER_VAAPI_HEVC, videoEncoder),
    CONFIG_INT(videoProfile, FF_PROFILE_H264_BASELINE, 0, INT_MAX, videoProfile),
    CONFIG_CONST(baseline, FF_PROFILE_H264_BASELINE, videoProfile),
    CONFIG_CONST(main, FF_PROFILE_H264_MAIN, videoProfile),
    CONFIG_CONST(high, FF_PROFILE_H264_HIGH, videoProfile),
    CONFIG_INT(videoPreset, RS_CONFIG_PRESET_FAST, RS_CONFIG_PRESET_FAST,
               RS_CONFIG_PRESET_SLOW, videoPreset),
    CONFIG_CONST(fast, RS_CONFIG_PRESET_FAST, videoPreset),
    CONFIG_CONST(medium, RS_CONFIG_PRESET_MEDIUM, videoPreset),
    CONFIG_CONST(slow, RS_CONFIG_PRESET_SLOW, videoPreset),
    CONFIG_INT(videoQuality, 28, RS_CONFIG_AUTO, 51, auto),
    CONFIG_INT64(videoBitrate, RS_CONFIG_AUTO, RS_CONFIG_AUTO, INT_MAX, auto),
    CONFIG_INT(videoGOP, 30, 0, INT_MAX, videoGOP),
    CONFIG_INT(scaleWidth, RS_CONFIG_AUTO, RS_CONFIG_AUTO, INT_MAX, auto),
    CONFIG_INT(scaleHeight, RS_CONFIG_AUTO, RS_CONFIG_AUTO, INT_MAX, auto),
    CONFIG_INT(audioInput, RS_CONFIG_AUTO, RS_CONFIG_DEVICE_NONE, RS_CONFIG_DEVICE_PULSE,
               audioInput),
    CONFIG_CONST(none, RS_CONFIG_DEVICE_NONE, audioInput),
    CONFIG_CONST(auto, RS_CONFIG_AUTO, audioInput),
    CONFIG_CONST(pulse, RS_CONFIG_DEVICE_PULSE, audioInput),
    CONFIG_STRING(audioDevice, "auto"),
    CONFIG_INT(audioSamplerate, 44100, 1, INT_MAX, auto),
    CONFIG_INT(audioEncoder, RS_CONFIG_AUTO, RS_CONFIG_AUTO, RS_CONFIG_ENCODER_FDK,
               audioEncoder),
    CONFIG_CONST(auto, RS_CONFIG_AUTO, audioEncoder),
    CONFIG_CONST(aac, RS_CONFIG_ENCODER_AAC, audioEncoder),
    CONFIG_CONST(fdk, RS_CONFIG_ENCODER_FDK, audioEncoder),
    CONFIG_INT(audioProfile, FF_PROFILE_AAC_LOW, 0, INT_MAX, audioProfile),
    CONFIG_CONST(low, FF_PROFILE_AAC_LOW, audioProfile),
    CONFIG_CONST(main, FF_PROFILE_AAC_MAIN, audioProfile),
    CONFIG_CONST(high, FF_PROFILE_AAC_HE, audioProfile),
    CONFIG_INT64(audioBitrate, RS_CONFIG_AUTO, RS_CONFIG_AUTO, INT_MAX, auto),
    CONFIG_INT(controller, RS_CONFIG_AUTO, RS_CONFIG_AUTO, RS_CONFIG_CONTROL_COMMAND,
               controller),
    CONFIG_CONST(auto, RS_CONFIG_AUTO, controller),
    CONFIG_CONST(debug, RS_CONFIG_CONTROL_DEBUG, controller),
    CONFIG_CONST(x11, RS_CONFIG_CONTROL_X11, controller),
    CONFIG_CONST(command, RS_CONFIG_CONTROL_COMMAND, controller),
    CONFIG_STRING(keyName, "r"),
    CONFIG_FLAGS(keyMods, RS_CONFIG_KEYMOD_CTRL | RS_CONFIG_KEYMOD_SUPER, keyMods),
    CONFIG_CONST(ctrl, RS_CONFIG_KEYMOD_CTRL, keyMods),
    CONFIG_CONST(shift, RS_CONFIG_KEYMOD_SHIFT, keyMods),
    CONFIG_CONST(alt, RS_CONFIG_KEYMOD_ALT, keyMods),
    CONFIG_CONST(super, RS_CONFIG_KEYMOD_SUPER, keyMods),
    CONFIG_STRING(outputFile, "~/Videos/ReplaySorcery/%F_%H-%M-%S.mp4"),
    CONFIG_STRING(outputCommand, "notify-send " RS_NAME " \"Saved replay as %s\""),
    {NULL}};

static const AVClass configClass = {
    .class_name = RS_NAME,
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

static int configParse(const char *path) {
   int ret;
   AVIOContext *file;
   if ((ret = avio_open(&file, path, AVIO_FLAG_READ)) < 0) {
      av_log(NULL, AV_LOG_WARNING, "Failed to open config file '%s': %s\n", path,
             av_err2str(ret));
      return 0;
   }

   av_log(NULL, AV_LOG_INFO, "Reading config file '%s'...\n", path);
   AVBPrint buffer;
   av_bprint_init(&buffer, 0, AV_BPRINT_SIZE_UNLIMITED);
   char *contents = NULL;
   if ((ret = avio_read_to_bprint(file, &buffer, INT_MAX)) < 0) {
      av_log(NULL, AV_LOG_ERROR, "Failed to read config file: %s\n", av_err2str(ret));
      goto error;
   }
   avio_closep(&file);

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
         ret = AVERROR_INVALIDDATA;
         goto error;
      }

      *eq = 0;
      char *key = configTrim(line);
      char *value = configTrim(eq + 1);
      av_log(NULL, AV_LOG_INFO, "Setting '%s' to '%s'...\n", key, value);
      if ((ret = av_opt_set(&rsConfig, key, value, 0)) < 0) {
         av_log(NULL, AV_LOG_ERROR, "Failed to set '%s' to '%s': %s\n", key, value,
                av_err2str(ret));
         goto error;
      }
      av_log_set_level(rsConfig.logLevel);
   }

   av_freep(&contents);
   return 0;
error:
   av_freep(&contents);
   av_bprint_finalize(&buffer, NULL);
   avio_closep(&file);
   rsConfigExit();
   return ret;
}

int rsConfigInit(void) {
   int ret;
   av_opt_set_defaults(&rsConfig);
   if ((ret = configParse(RS_BUILD_GLOBAL_CONFIG)) < 0) {
      return ret;
   }

   const char *home = getenv("HOME");
   if (home == NULL) {
      av_log(NULL, AV_LOG_ERROR, "Failed to get $HOME variable\n");
      return AVERROR_EXTERNAL;
   }

   char *local = rsFormat(RS_BUILD_LOCAL_CONFIG, home);
   if (local == NULL) {
      return AVERROR(ENOMEM);
   }

   ret = configParse(local);
   av_freep(&local);
   if (ret < 0) {
      return ret;
   }
   return 0;
}

void rsConfigExit(void) {
   av_opt_free(&rsConfig);
}
