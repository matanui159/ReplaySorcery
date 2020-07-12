/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "main.h"
#include "config/config.h"
#include "error.h"
#include "record.h"
#include "save.h"
#include "user/default.h"
#include "user/user.h"
#include <libavutil/avutil.h>
#include <signal.h>
#include <stdlib.h>

static struct {
   /**
    * The user object to wait on before saving the video.
    */
   RSUser user;
} priv;

/**
 * Signal handler for termination requests.
 */
static void mainSignal(int signal) {
   (void)signal;
   rsMainExit();
}

void rsMainExit(void) {
   rsUserDestroy(&priv.user);
   rsSaveExit();
   rsRecordExit();
   exit(EXIT_SUCCESS);
}

int main(int argv, char **argc) {
   (void)argc;
   (void)argv;
   signal(SIGINT, mainSignal);
   signal(SIGTERM, mainSignal);

   rsConfigInit();
   rsErrorInit();
   rsRecordInit();
   rsSaveInit();
   rsDefaultUserCreate(&priv.user);
   for (;;) {
      rsUserWait(&priv.user);
      rsSave();
   }
   return 0;
}
