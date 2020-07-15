/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "system.h"
#include "../util/memory.h"

void rsSystemDestroy(RSSystem *system) {
   system->destroy(system);
   rsMemoryClear(system, sizeof(RSSystem));
}

void rsSystemFrameCreate(RSFrame *frame, RSSystem *system) {
   system->frameCreate(frame, system);
}

bool rsSystemWantsSave(RSSystem *system) {
   return system->wantsSave(system);
}
