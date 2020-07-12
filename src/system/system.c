/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "system.h"

void rsSystemDestroy(RSSystem *system) {
   system->destroy(system);
   rsClear(system, sizeof(RSSystem));
}

void rsSystemGetFrame(RSSystem *system, RSSystemFrame *frame) {
   system->getFrame(system, frame);
}

bool rsSystemWantsSave(RSSystem *system) {
   return system->wantsSave(system);
}
