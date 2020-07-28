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
#include "util/buffer.h"
#include "util/log.h"
#include "util/memory.h"
#include "util/path.h"
#include "util/string.h"
#include <ctype.h>
#include <errno.h>
#include <limits.h>

static bool rsConfigLoadFile(RSConfig *config, const char *fn);
static bool rsConfigLoadDir(RSConfig *config, const char *dirs);
static void rsConfigParseLine(RSConfig *config, char *line);
static void rsConfigParseFile(RSConfig *config, FILE *f);
static void configInt(void *param, const char *value);
static void configString(void *param, const char *value);
static void configSet(RSConfig *config, const char *key, const char *value);
static void rsConfigSetDefaults(RSConfig *config);

#define CONFIG_NAME "/replay-sorcery.conf"

typedef struct ConfigParam {
   const char *key;
   size_t offset;
   void (*set)(void *param, const char *value);
   const char *def;
} ConfigParam;

#define CONFIG_PARAM(key, set, def)                                                      \
   { #key, offsetof(RSConfig, key), set, def }

// Remember to update `replay-sorcery.default.conf`
static const ConfigParam configParams[] = {
    CONFIG_PARAM(offsetX, configInt, "0"),
    CONFIG_PARAM(offsetY, configInt, "0"),
    CONFIG_PARAM(width, configInt, "1920"),
    CONFIG_PARAM(height, configInt, "1080"),
    CONFIG_PARAM(framerate, configInt, "30"),
    CONFIG_PARAM(duration, configInt, "30"),
    CONFIG_PARAM(compressQuality, configInt, "70"),
    CONFIG_PARAM(outputFile, configString, "~/Videos/ReplaySorcery_%F_%H-%M-%S.mp4"),
    CONFIG_PARAM(preOutputCommand, configString, ""),
    CONFIG_PARAM(postOutputCommand, configString,
                 "notify-send ReplaySorcery \"Video saved!\"")};

#define CONFIG_PARAMS_SIZE (sizeof(configParams) / sizeof(ConfigParam))

void rsConfigLoad(RSConfig *config) {
   rsConfigSetDefaults(config);
   char *extra_buffer = 0;
   char *dirs = getenv("XDG_CONFIG_DIRS");
   if (dirs == NULL) {
      dirs = "/etc/xdg";
   }
   if (rsConfigLoadDir(config, dirs)) {
      return;
   }
   dirs = getenv("XDG_CONFIG_HOME");
   if (dirs == NULL) {
      //~/... will not work for fopen
      dirs = getenv("HOME");
      if (dirs == NULL) {
         // not even home is set... just try to load from /etc/replay-sorcery.conf
         dirs = "/etc/replay-sorcery.conf";
         rsConfigLoadDir(config, dirs);
         return;
      }
      // add $HOME/.config
      const char *dotconf = "/.config";
      size_t homelen = strlen(dirs);
      // totallen = home + : + home + dotconf + null
      size_t totallen = homelen * 2 + strlen(dotconf) + 2;
      extra_buffer = rsMemoryCreate(totallen);
      memcpy(extra_buffer, dirs, homelen);
      extra_buffer[homelen] = ':';
      memcpy(extra_buffer + homelen + 1, dirs, homelen);
      strcpy(extra_buffer + homelen * 2 + 1, dotconf);
      dirs = extra_buffer;
   }
   rsConfigLoadDir(config, dirs);
   if (extra_buffer) {
      rsMemoryDestroy(extra_buffer);
   }
}

void rsConfigDestroy(RSConfig *config) {

   rsMemoryDestroy(config->outputFile);
   rsMemoryDestroy(config->preOutputCommand);
   rsMemoryDestroy(config->postOutputCommand);
}

static bool rsConfigLoadDir(RSConfig *config, const char *dirs) {
   bool loaded = false;
   size_t size = strlen(dirs) + 1;
   char *buffer = rsMemoryCreate(size);
   memcpy(buffer, dirs, size);
   char *pch = strtok(buffer, ":");
   while (pch != NULL) {
      size_t sizedir = strlen(pch);
      char *bufferdir = rsMemoryCreate(sizedir + strlen(CONFIG_NAME) + 1);
      memcpy(bufferdir, pch, sizedir);
      strcpy(bufferdir + sizedir, CONFIG_NAME);
      loaded = rsConfigLoadFile(config, bufferdir);
      rsMemoryDestroy(bufferdir);
      if (loaded) {
         break;
      }
      pch = strtok(NULL, ":");
   }
   rsMemoryDestroy(buffer);
   return loaded;
}

static bool rsConfigLoadFile(RSConfig *config, const char *fn) {
   FILE *f = fopen(fn, "r");
   if (!f) {
      return false;
   }
   rsLog("Loading config file: %s", fn);
   rsConfigParseFile(config, f);
   fclose(f);
   return true;
}

static void rsConfigParseLine(RSConfig *config, char *line) {
   // remove comments (#) and newlines (\n)
   char *found = strchr(line, '#');
   if (found != NULL) {
      *found = '\0';
   }
   found = strchr(line, '\n');
   if (found != NULL) {
      *found = '\0';
   }
   found = strchr(line, '=');
   if (found == NULL) {
      rsError(
          "Failed to load config line, format is <key> = <value> # <optional comment>");
   }
   *found = '\0';
   char *key = rsStringTrimEnd(line);
   char *value = rsStringTrimStart(found + 1);
   configSet(config, key, value);
   line[0] = '\0';
}

static void rsConfigParseFile(RSConfig *config, FILE *f) {
   char linebuf[1024 * 4];
   while (!feof(f)) {
      fgets(linebuf, sizeof(linebuf), f);
      if (linebuf[0] == 0 || linebuf[0] == '#' || linebuf[0] == '\n') {
         continue;
      }
      rsConfigParseLine(config, linebuf);
   }
}

static void configInt(void *param, const char *value) {
   int *num = param;
   char *end;
   long ret = strtol(value, &end, 10);
   if (*end != '\0' || ret < INT_MIN || ret > INT_MAX) {
      rsError("Config value '%s' is not a valid integer", value);
   }
   *num = (int)ret;
}

static void configString(void *param, const char *value) {
   char **str = param;
   rsMemoryDestroy(*str);
   size_t size = strlen(value) + 1;
   *str = rsMemoryCreate(size);
   memcpy(*str, value, size);
}

static void configSet(RSConfig *config, const char *key, const char *value) {
   rsLog("Setting config key '%s' to '%s'...", key, value);
   for (size_t i = 0; i < CONFIG_PARAMS_SIZE; ++i) {
      if (strcmp(configParams[i].key, key) == 0) {
         void *param = (char *)config + configParams[i].offset;
         configParams[i].set(param, value);
         return;
      }
   }
   rsLog("Config key '%s' does not exist", key);
}

static void rsConfigSetDefaults(RSConfig *config) {
   rsLog("Loading defaults...");
   rsMemoryClear(config, sizeof(RSConfig));
   for (size_t i = 0; i < CONFIG_PARAMS_SIZE; ++i) {
      configSet(config, configParams[i].key, configParams[i].def);
   }
}
