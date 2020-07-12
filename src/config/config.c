/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "config.h"
#include <limits.h>
#include <stdlib.h>

typedef struct ConfigParam {
   const char *key;
   size_t offset;
   void (*set)(void *param, const char *value);
   const char *def;
} ConfigParam;

#define CONFIG_PARAM(key, set, def)                                                      \
   { #key, offsetof(RSConfig, key), set, def }

static void setConfigInt(void *param, const char *value) {
   int *num = param;
   char *end;
   long ret = strtol(value, &end, 10);
   if (*end != '\0' || ret < INT_MIN || ret > INT_MAX) {
      rsError(0, "Config value '%s' is not a valid integer", value);
   }
   *num = (int)ret;
}

static const ConfigParam configParams[] = {
    CONFIG_PARAM(inputX, setConfigInt, "0"), CONFIG_PARAM(inputY, setConfigInt, "0"),
    CONFIG_PARAM(inputWidth, setConfigInt, "1920"),
    CONFIG_PARAM(inputHeight, setConfigInt, "1080"),
    CONFIG_PARAM(compressQuality, setConfigInt, "100")};

void rsConfigDefaults(RSConfig *config) {
   for (size_t i = 0; i < RS_ARRAY_SIZE(configParams); ++i) {
      rsConfigSet(config, configParams[i].key, configParams[i].def);
   }
}

void rsConfigSet(RSConfig *config, const char *key, const char *value) {
   for (size_t i = 0; i < RS_ARRAY_SIZE(configParams); ++i) {
      if (strcmp(configParams[i].key, key) == 0) {
         void *param = (char *)config + configParams[i].offset;
         configParams[i].set(param, value);
         break;
      }
   }
}
