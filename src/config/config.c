/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "config.h"
#include "../util/log.h"
#include <limits.h>

typedef struct ConfigParam {
   const char *key;
   size_t offset;
   void (*set)(void *param, const char *value);
   const char *def;
} ConfigParam;

#define CONFIG_PARAM(key, set, def)                                                      \
   { #key, offsetof(RSConfig, key), set, def }

static void configInt(void *param, const char *value) {
   int *num = param;
   char *end;
   long ret = strtol(value, &end, 10);
   if (*end != '\0' || ret < INT_MIN || ret > INT_MAX) {
      rsError("Config value '%s' is not a valid integer", value);
   }
   *num = (int)ret;
}

static const ConfigParam configParams[] = {
    CONFIG_PARAM(offsetX, configInt, "0"),
    CONFIG_PARAM(offsetY, configInt, "0"),
    CONFIG_PARAM(width, configInt, "1920"),
    CONFIG_PARAM(height, configInt, "1080"),
    CONFIG_PARAM(framerate, configInt, "30"),
    CONFIG_PARAM(duration, configInt, "30"),
    CONFIG_PARAM(compressQuality, configInt, "70")};

#define CONFIG_PARAMS_SIZE (sizeof(configParams) / sizeof(ConfigParam))

void rsConfigDefaults(RSConfig *config) {
   for (size_t i = 0; i < CONFIG_PARAMS_SIZE; ++i) {
      rsConfigSet(config, configParams[i].key, configParams[i].def);
   }
}

void rsConfigSet(RSConfig *config, const char *key, const char *value) {
   for (size_t i = 0; i < CONFIG_PARAMS_SIZE; ++i) {
      if (strcmp(configParams[i].key, key) == 0) {
         void *param = (char *)config + configParams[i].offset;
         configParams[i].set(param, value);
         break;
      }
   }
}
