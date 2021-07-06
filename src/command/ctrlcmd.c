/*
 * Copyright (C) 2021  Joshua Minter
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
#include "../socket.h"
#include "command.h"

int rsControlSave(void) {
   int ret;
   RSSocket sock = {0};
   if ((ret = rsSocketCreate(&sock)) < 0) {
      goto error;
   }
   if ((ret = rsSocketConnect(&sock, RS_COMMAND_CONTROL_PATH)) < 0) {
      goto error;
   }

   ret = 0;
error:
   rsSocketDestroy(&sock);
   return ret;
}
