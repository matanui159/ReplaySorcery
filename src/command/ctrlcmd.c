/*
 * Copyright (C) 2020  <NAME>
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

#include "../control/control.h"
#include "command.h"
#include "rsbuild.h"
#ifdef RS_BUILD_UNIX_SOCKET_FOUND
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#endif

int rsControlSave(void) {
#ifdef RS_BUILD_UNIX_SOCKET_FOUND
   int ret;
   int fd = socket(AF_UNIX, SOCK_STREAM, 0);
   if (fd == -1) {
      ret = AVERROR(errno);
      av_log(NULL, AV_LOG_ERROR, "Failed to create socket: %s\n", av_err2str(ret));
      goto error;
   }

   struct sockaddr_un addr = {.sun_family = AF_UNIX, .sun_path = RS_COMMAND_CONTROL_PATH};
   if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
      ret = AVERROR(errno);
      av_log(NULL, AV_LOG_ERROR, "Failed to connect socket: %s\n", av_err2str(ret));
      goto error;
   }

   ret = 0;
error:
   if (fd != -1) {
      close(fd);
   }
   return ret;

#else
   av_log(NULL, AV_LOG_ERROR, "Unix socket was not found during compilation\n");
   return AVERROR(ENOSYS);
#endif
}
