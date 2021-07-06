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

#include "../socket.h"
#include "control.h"

static void commandControlDestroy(RSControl *control) {
   RSSocket *sock = control->extra;
   if (sock != NULL) {
      rsSocketDestroy(sock);
      av_freep(&control->extra);
   }
}

static int commandControlWantsSave(RSControl *control) {
   int ret;
   RSSocket *sock = control->extra;
   RSSocket conn = {0};
   if ((ret = rsSocketAccept(sock, &conn, 0)) < 0) {
      if (ret == AVERROR(EAGAIN)) {
         return 0;
      } else {
         return ret;
      }
   }
   rsSocketDestroy(&conn);
   return 1;
}

int rsCommandControlCreate(RSControl *control) {
   int ret;
   RSSocket *sock = av_mallocz(sizeof(RSSocket));
   control->extra = sock;
   control->destroy = commandControlDestroy;
   control->wantsSave = commandControlWantsSave;
   if (sock == NULL) {
      ret = AVERROR(ENOMEM);
      goto error;
   }
   if ((ret = rsSocketCreate(sock)) < 0) {
      goto error;
   }
   if ((ret = rsSocketBind(sock, RS_COMMAND_CONTROL_PATH)) < 0) {
      goto error;
   }

   return 0;
error:
   rsControlDestroy(control);
   return ret;
}
