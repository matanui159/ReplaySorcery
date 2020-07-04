/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef SR_USER_H
#define SR_USER_H

typedef struct SRUser {
   void (*wait)(struct SRUser* user);
   void (*destroy)(struct SRUser* user);
   void* extra;
} SRUser;

void srUserDestroy(SRUser* user);
void srUserWait(SRUser* user);

#endif
