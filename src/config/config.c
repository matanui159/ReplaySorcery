/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "config.h"
#include "../error.h"
#include "libavutil/opt.h"
#include "paths.h"

/**
 * A utility macro to get the offset into the configuration object.
 */
#define OFFSET(name) offsetof(RSConfig, name)

/**
 * The avutil class implementation and its option mappings.
 */
// clang-format off
// <name>, <description>, <offset>, <type>, <default>, <min>, <max>, <flags>, <unit>
static AVOption configOptions[] = {
   { "inputDisplay", NULL, OFFSET(inputDisplay), AV_OPT_TYPE_STRING, {.str=":0.0"}, 0, 0, 0, NULL },
   { "inputWidth", NULL, OFFSET(inputWidth), AV_OPT_TYPE_INT, {.i64=1920}, 1, INT_MAX, 0, NULL },
   { "inputHeight", NULL, OFFSET(inputHeight), AV_OPT_TYPE_INT, {.i64=1080}, 1, INT_MAX, 0, NULL, },
   { "inputX", NULL, OFFSET(inputX), AV_OPT_TYPE_INT, {.i64=0}, 0, INT_MAX, 0, NULL },
   { "inputY", NULL, OFFSET(inputY), AV_OPT_TYPE_INT, {.i64=0}, 0, INT_MAX, 0, NULL },
   { "inputFramerate", NULL, OFFSET(inputFramerate), AV_OPT_TYPE_INT, {.i64=30}, 1, INT_MAX, 0, NULL },
   { "recordWidth", NULL, OFFSET(recordWidth), AV_OPT_TYPE_INT, {.i64=-1}, -1, INT_MAX, 0, NULL },
   { "recordHeight", NULL, OFFSET(recordHeight), AV_OPT_TYPE_INT, {.i64=-1}, -1, INT_MAX, 0, NULL },
   { "recordQuality", NULL, OFFSET(recordQuality), AV_OPT_TYPE_INT, {.i64=RS_QUALITY_LOW}, RS_QUALITY_LOW, RS_QUALITY_HIGH, 0, "quality" },
      { "low", NULL, 0, AV_OPT_TYPE_CONST, {.i64=RS_QUALITY_LOW}, 0, 0, 0, "quality" },
      { "medium", NULL, 0, AV_OPT_TYPE_CONST, {.i64=RS_QUALITY_MEDIUM}, 0, 0, 0, "quality" },
      { "high", NULL, 0, AV_OPT_TYPE_CONST, {.i64=RS_QUALITY_HIGH}, 0, 0, 0, "quality" },
   { "recordDuration", NULL, OFFSET(recordDuration), AV_OPT_TYPE_INT, {.i64=30}, 1, INT_MAX, 0, NULL },
   { "disableX11Input", NULL, OFFSET(disableX11Input), AV_OPT_TYPE_BOOL, {.i64=false}, false, true, 0, NULL },
   { "disableNvidiaEncoder", NULL, OFFSET(disableNvidiaEncoder), AV_OPT_TYPE_BOOL, {.i64=false}, false, true, 0, NULL },
   { "disableVaapiEncoder", NULL, OFFSET(disableVaapiEncoder), AV_OPT_TYPE_BOOL, {.i64=false}, false, true, 0, NULL },
   { "disableSoftwareEncoder", NULL, OFFSET(disableSoftwareEncoder), AV_OPT_TYPE_BOOL, {.i64=false}, false, true, 0, NULL },
   { "outputFile", NULL, OFFSET(outputFile), AV_OPT_TYPE_STRING, {.str="~/Videos/ReplaySorcery_%F_%H-%M-%S.mp4"}, 0, 0, 0, NULL },
   { "logLevel", NULL, OFFSET(logLevel), AV_OPT_TYPE_INT, {.i64=AV_LOG_VERBOSE}, AV_LOG_QUIET, AV_LOG_TRACE, 0, "log" },
      { "quiet", NULL, 0, AV_OPT_TYPE_CONST, {.i64=AV_LOG_QUIET}, 0, 0, 0, "log" },
      { "panic", NULL, 0, AV_OPT_TYPE_CONST, {.i64=AV_LOG_PANIC}, 0, 0, 0, "log" },
      { "fatal", NULL, 0, AV_OPT_TYPE_CONST, {.i64=AV_LOG_FATAL}, 0, 0, 0, "log" },
      { "error", NULL, 0, AV_OPT_TYPE_CONST, {.i64=AV_LOG_ERROR}, 0, 0, 0, "log" },
      { "warning", NULL, 0, AV_OPT_TYPE_CONST, {.i64=AV_LOG_WARNING}, 0, 0, 0, "log" },
      { "info", NULL, 0, AV_OPT_TYPE_CONST, {.i64=AV_LOG_INFO}, 0, 0, 0, "log" },
      { "verbose", NULL, 0, AV_OPT_TYPE_CONST, {.i64=AV_LOG_VERBOSE}, 0, 0, 0, "log" },
      { "debug", NULL, 0, AV_OPT_TYPE_CONST, {.i64=AV_LOG_DEBUG}, 0, 0, 0, "log" },
      { "trace", NULL, 0, AV_OPT_TYPE_CONST, {.i64=AV_LOG_TRACE}, 0, 0, 0, "qualogity" },
   { "enableDebug", NULL, OFFSET(enableDebug), AV_OPT_TYPE_BOOL, {.i64=false}, false, true, 0, NULL },
   { NULL, NULL, 0, 0, {.i64=0}, 0, 0, 0, NULL }
};

static AVClass configClass = {
   .class_name = "ReplaySorcery",
   .item_name = av_default_item_name,
   .version = LIBAVUTIL_VERSION_INT,
   .option = configOptions
};
// clang-format on

RSConfig rsConfig = {.avclass = &configClass};

void rsConfigInit(void) {
   // Use default until we know what level to set it to.
   av_log_set_level(AV_LOG_VERBOSE);
   av_opt_set_defaults(&rsConfig);
   rsConfigPathsRead();

   // Fixup width and height if set to -1 (auto).
   if (rsConfig.recordWidth == -1) rsConfig.recordWidth = rsConfig.inputWidth;
   if (rsConfig.recordHeight == -1) rsConfig.recordHeight = rsConfig.inputHeight;
   av_log_set_level(rsConfig.logLevel);
}
