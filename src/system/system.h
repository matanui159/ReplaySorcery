/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef RS_SYSTEM_H
#define RS_SYSTEM_H
#include "../std.h"
#include "../util/frame.h"

typedef struct RSSystem {
   void *extra;
   void (*destroy)(struct RSSystem *system);
   void (*frameCreate)(RSFrame *frame, struct RSSystem *system);
   bool (*wantsSave)(struct RSSystem *system);
} RSSystem;

void rsSystemDestroy(RSSystem *system);
void rsSystemFrameCreate(RSFrame *frame, RSSystem *system);
bool rsSystemWantsSave(RSSystem *system);

#endif
