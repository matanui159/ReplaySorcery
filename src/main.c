#include "error.h"
#include "record.h"
#include "save.h"
#include "user/user.h"
#include "user/default.h"
#include <stdlib.h>

static struct {
   SRUser user;
} priv;

static void mainExit(void) {
   srUserDestroy(&priv.user);
   srSaveExit();
   srRecordExit();
   exit(EXIT_SUCCESS);
}

int main(int argv, char** argc) {
   (void)argc;
   (void)argv;
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
