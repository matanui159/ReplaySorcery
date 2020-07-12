/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef RS_SYSTEM_H
#define RS_SYSTEM_H
#include "../common.h"

typedef struct RSSystemFrame {
   const uint8_t *data;
   size_t width;
   size_t height;
   size_t stride;
} RSSystemFrame;

typedef struct RSSystem {
   void *extra;
   void (*destroy)(struct RSSystem *system);
   void (*getFrame)(struct RSSystem *system, RSSystemFrame *frame);
   bool (*wantsSave)(struct RSSystem *system);
} RSSystem;

void rsSystemDestroy(RSSystem *system);
void rsSystemGetFrame(RSSystem *system, RSSystemFrame *frame);
bool rsSystemWantsSave(RSSystem *system);

#endif
