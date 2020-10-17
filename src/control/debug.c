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

#include "control.h"
#include "rsbuild.h"
#include <errno.h>
#ifdef RS_BUILD_UNISTD_FOUND
#include <unistd.h>
#endif
#ifdef RS_BUILD_FCNTL_FOUND
#include <fcntl.h>
#endif

static int debugControlWantsSave(RSControl *control) {
   (void)control;
   int ret = 0;
   for (;;) {
      char c;
      ssize_t result = read(0, &c, 1);
      if (result < 0) {
         if (errno != EAGAIN) {
            ret = AVERROR(errno);
            av_log(NULL, AV_LOG_ERROR, "Failed to read from stdin: %s\n", av_err2str(ret));
         }
         return ret;
      }
      if (result == 1 && c == '\n') {
         ret = 1;
      }
   }
}

int rsDebugControlCreate(RSControl *control) {
#if defined(RS_BUILD_UNISTD_FOUND) && defined(RS_BUILD_FCNTL_FOUND)
   int flags = fcntl(0, F_GETFL);
   fcntl(0, F_SETFL, flags | O_NONBLOCK);
   control->wantsSave = debugControlWantsSave;
   return 0;

#else
   (void)control;
#ifndef RS_BUILD_UNISTD_FOUND
   av_log(NULL, AV_LOG_ERROR, "<unistd.h> was not found during compilation\n");
#endif
#ifndef RS_BUILD_FCNTL_FOUND
   av_log(NULL, AV_LOG_ERROR, "<fcntl.h> was not found during compilation\n");
#endif
   return AVERROR(ENOSYS);
#endif
}
