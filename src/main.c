/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "main.h"
#include "error.h"
#include "record.h"
#include "save.h"
#include "user/user.h"
#include "user/default.h"
#include <stdlib.h>
#include <signal.h>

static struct {
   /**
    * The user object to wait on before saving the video.
    */
   SRUser user;
} priv;

/**
 * Signal handler for termination requests.
 */
static void mainSignal(int signal) {
   (void)signal;
   srMainExit();
}

void srMainExit(void) {
   srUserDestroy(&priv.user);
   srSaveExit();
   srRecordExit();
   exit(EXIT_SUCCESS);
}

int main(int argv, char** argc) {
   (void)argc;
   (void)argv;
   signal(SIGINT, mainSignal);
   signal(SIGTERM, mainSignal);

   srErrorInit();
   srRecordInit();
   srSaveInit();
   srDefaultUserCreate(&priv.user);
   for (;;) {
      srUserWait(&priv.user);
      srSave();
   }
   return 0;
}
