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
#include "device/device.h"
#include "util/log.h"
#include <libavutil/avutil.h>

static int mainRun(void) {
   int ret;
   rsLogInit();
   if ((ret = rsConfigInit()) < 0) {
      return ret;
   }
   rsDeviceInit();

   av_log(NULL, AV_LOG_INFO,
          "ReplaySorcery  Copyright (C) 2020  ReplaySorcery developers\n"
          "This program comes with ABSOLUTELY NO WARRANTY.\n"
          "This is free software, and you are welcome to redistribute it\n"
          "under certain conditions; see COPYING for details.\n");
   av_log(NULL, AV_LOG_INFO, "FFmpeg version: %s\n", av_version_info());

   RSDevice device;
   if ((ret = rsVideoDeviceCreate(&device)) < 0) {
      return ret;
   }
   rsDeviceDestroy(&device);
   return 0;
}

int main(int argc, char *argv[]) {
   (void)argc;
   (void)argv;
   int ret;
   if ((ret = mainRun()) < 0) {
      av_log(NULL, AV_LOG_FATAL, "%s\n", av_err2str(ret));
      return EXIT_FAILURE;
   }
   return EXIT_SUCCESS;
}
