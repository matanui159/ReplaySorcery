/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "user.h"
#include <stdlib.h>

void rsUserDestroy(RSUser *user) {
   if (user->destroy != NULL) {
      user->destroy(user);
   }
}

void rsUserWait(RSUser *user) {
   if (user->wait != NULL) {
      user->wait(user);
   }
}
