/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "file.h"
#include "../error.h"
#include "config.h"
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <libavutil/avstring.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/**
 * Finds the first non-space character.
 */
static char *configFileTrimStart(char *str) {
   while (isspace(*str)) {
      ++str;
   }
   return str;
}

/**
 * Finds the last non-space character and NULL-terminates the string after it.
 */
static char *configFileTrimEnd(char *str) {
   // `strchr` can be used to find the NULL terminated end of a string.
   char *end = strchr(str, '\0') - 1;
   while (isspace(*end)) {
      --end;
   }
   // Set the character after the last non-space character to NULL.
   end[1] = '\0';
   return str;
}

/**
 * Parses a single line from a config file. Heavily modifies the line so the line should
 * not be used after being passed into this function.
 */
static void configFileLine(char *line) {
   int ret;
   // Remove comment on line if there is one.
   char *comment = strchr(line, '#');
   if (comment != NULL)
      *comment = '\0';

   // Check if empty line by trimming first.
   line = configFileTrimEnd(configFileTrimStart(line));
   if (*line == '\0')
      return;

   // Split by `=`.
   char *eq = strchr(line, '=');
   if (eq == NULL) {
      av_log(NULL, AV_LOG_WARNING, "Invalid config line: %s\n", line);
      return;
   }
   *eq = '\0';
   // We do not need to trim from both ends since the other ends was already trimmed
   // earlier.
   char *key = configFileTrimEnd(line);
   char *value = configFileTrimStart(eq + 1);

   // Set the value.
   if ((ret = av_opt_set(&rsConfig, key, value, 0)) < 0) {
      av_log(NULL, AV_LOG_WARNING, "Failed to set '%s' to '%s': %s\n", key, value,
             av_err2str(ret));
   }
}

void rsConfigFileRead(const char *path) {
   av_log(NULL, AV_LOG_INFO, "Reading config from '%s'...\n", path);
   int fd = open(path, O_RDONLY);
   if (fd < 0) {
      av_log(NULL, AV_LOG_WARNING, "Failed to read '%s': %s\n", path,
             av_err2str(AVERROR(errno)));
      return;
   }

   // Read the whole file into a VLA and set a NULL terminator.
   struct stat stats;
   rsCheckErrno(fstat(fd, &stats));
   char contents[stats.st_size + 1];
   ssize_t size = read(fd, contents, (size_t)stats.st_size);
   contents[size] = '\0';
   close(fd);

   // Read line by line.
   char *state = NULL;
   char *line = av_strtok(contents, "\n", &state);
   while (line != NULL) {
      configFileLine(line);
      line = av_strtok(NULL, "\n", &state);
   }
}
