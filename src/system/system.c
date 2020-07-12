/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "system.h"

void rsDestroySystem(RSSystem *system) {
   if (system->destroy != NULL) {
      system->destroy(system);
   }
   rsClear(system, sizeof(RSSystem));
}

void rsGetSystemFrame(RSSystem *system, RSSystemFrame *frame) {
   if (system->getFrame != NULL) {
      system->getFrame(system, frame);
   } else {
      rsClear(frame, sizeof(RSSystemFrame));
   }
}

bool rsSystemWantsSave(RSSystem *system) {
   if (system->wantsSave != NULL) {
      return system->wantsSave(system);
   }
   return false;
}
