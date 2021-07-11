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

#include "control.h"
#include "rsbuild.h"
#include <errno.h>
#ifdef RS_BUILD_POSIX_IO_FOUND
#include <fcntl.h>
#include <unistd.h>
#endif

#ifdef RS_BUILD_POSIX_IO_FOUND
static int debugControlWantsSave(RSControl *control) {
   (void)control;
   int ret = 0;
   for (;;) {
      char c;
      ssize_t result = read(0, &c, 1);
      if (result == -1) {
         if (errno != EAGAIN) {
            ret = AVERROR(errno);
            av_log(NULL, AV_LOG_ERROR, "Failed to read from stdin: %s\n",
                   av_err2str(ret));
         }
         return ret;
      }
      if (result == 1 && c == '\n') {
         ret = 1;
      }
   }
}
#endif

int rsDebugControlCreate(RSControl *control) {
#ifdef RS_BUILD_POSIX_IO_FOUND
   control->destroy = NULL;
   control->wantsSave = debugControlWantsSave;
   int flags = fcntl(0, F_GETFL);
   fcntl(0, F_SETFL, flags | O_NONBLOCK);
   return 0;

#else
   (void)control;
   av_log(NULL, AV_LOG_ERROR, "Posix I/O was not found during compilation\n");
   return AVERROR(ENOSYS);
#endif
}
