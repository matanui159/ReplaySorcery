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

#include "user.h"
#include "rsbuild.h"
#ifdef RS_BUILD_UNISTD_FOUND
#include <unistd.h>
#endif

static unsigned userReal;
static unsigned userEffective;

static int userSet(unsigned user) {
   int ret;
   if (userReal == userEffective) {
      // No need to ever switch
      return 0;
   }
   av_log(NULL, AV_LOG_DEBUG, "Switching to user %u...\n", user);
#ifdef RS_BUILD_SETEUID_FOUND
   if (seteuid(user) < 0) {
      ret = AVERROR(errno);
      av_log(NULL, AV_LOG_ERROR, "Failed to switch user: %s\n", av_err2str(ret));
      return ret;
   }
   return 0;

#else
   (void) user;
   av_log(NULL, AV_LOG_ERROR, "seteuid() was not found during compilation\n");
   return AVERROR(ENOSYS);
#endif
}

int rsUserInit(void) {
   int ret;
#ifdef RS_BUILD_GETUID_FOUND
   userReal = getuid();
#else
   userReal = 0;
#endif
#ifdef RS_BUILD_GETEUID_FOUND
   userEffective = geteuid();
#ifndef RS_BUILD_GETUID_FOUND
   userReal = userEffective;
#endif
#else
   userEffective = userReal;
#endif

   av_log(NULL, AV_LOG_INFO, "Real user ID: %u\n", userReal);
   av_log(NULL, AV_LOG_INFO, "Effective user ID: %u\n", userEffective);
   if ((ret = rsUserReal()) < 0) {
      return ret;
   }
   return 0;
}

int rsUserReal(void) {
   int ret;
   if ((ret = userSet(userReal)) < 0) {
      return ret;
   }
   return 0;
}

int rsUserEffective(void) {
   int ret;
   if ((ret = userSet(userEffective)) < 0) {
      return ret;
   }
   return 0;
}
