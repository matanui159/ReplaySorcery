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
#include "../config.h"

void rsControlDestroy(RSControl *control) {
   if (control->destroy != NULL) {
      control->destroy(control);
   }
}

int rsDefaultControlCreate(RSControl *control) {
   int ret;
   switch (rsConfig.controller) {
   case RS_CONFIG_CONTROL_DEBUG:
      return rsDebugControlCreate(control);
   case RS_CONFIG_CONTROL_X11:
      return rsX11ControlCreate(control);
   case RS_CONFIG_CONTROL_COMMAND:
      return rsCommandControlCreate(control);
   }

   if ((ret = rsX11ControlCreate(control)) >= 0) {
      av_log(NULL, AV_LOG_INFO, "Created X11 controller\n");
      return 0;
   }
   av_log(NULL, AV_LOG_WARNING, "Failed to create X11 controller: %s\n", av_err2str(ret));

   if ((ret = rsCommandControlCreate(control)) >= 0) {
      av_log(NULL, AV_LOG_INFO, "Created command controller\n");
      return 0;
   }
   av_log(NULL, AV_LOG_WARNING, "Failed to create command controller: %s\n",
          av_err2str(ret));

   return AVERROR(ENOSYS);
}
