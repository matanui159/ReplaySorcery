#include "user.h"
#include <stdlib.h>

void srUserDestroy(SRUser* user) {
   if (user->destroy != NULL) {
      user->destroy(user);
   }
}

void srUserWait(SRUser* user) {
   if (user->wait != NULL) {
      user->wait(user);
   }
}
