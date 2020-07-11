/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "paths.h"
#include "../path.h"
#include "file.h"
#include <libavutil/avstring.h>
#include <libavutil/avutil.h>
#include <libavutil/bprint.h>
#include <stdlib.h>

/**
 * The name of the config file inside the XDG paths.
 */
#define CONFIG_NAME "replay-sorcery.conf"

/**
 * Small utility to get an environment variable, or use a default if it does not exist.
 */
static const char* configPathsEnv(const char* name, const char* def) {
   const char* value = getenv(name);
   if (value == NULL) {
      av_log(NULL, AV_LOG_WARNING,
             "Environment variable '%s' does not exist, using default: %s\n", name, def);
      return def;
   }
   return value;
}

void rsConfigPathsRead(void) {
   AVBPrint path;
   av_bprint_init(&path, 0, AV_BPRINT_SIZE_AUTOMATIC);

   // Global directories. Multiple paths seperated by `:`. We have to duplicate it so we
   // can modify it for `av_strtok`.
   char* xdgDirs = av_strdup(configPathsEnv("XDG_CONFIG_DIRS", "/etc/xdg"));
   char* state = NULL;
   char* dir = av_strtok(xdgDirs, ":", &state);
   while (dir != NULL) {
      rsPathJoin(&path, dir, 0);
      rsPathJoin(&path, CONFIG_NAME, 0);
      rsConfigFileRead(path.str);
      av_bprint_clear(&path);
      dir = av_strtok(NULL, ":", &state);
   }
   av_freep(&xdgDirs);

   // Home directory.
   rsPathJoin(&path, configPathsEnv("XDG_CONFIG_HOME", "~/.config"), 0);
   rsPathJoin(&path, CONFIG_NAME, 0);
   rsConfigFileRead(path.str);
   av_bprint_finalize(&path, NULL);

   // Local directory.
   rsConfigFileRead(CONFIG_NAME);
}
