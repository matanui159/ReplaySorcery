/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef RS_USER_H
#define RS_USER_H

typedef struct RSUser {
   void (*wait)(struct RSUser* user);
   void (*destroy)(struct RSUser* user);
   void* extra;
} RSUser;

void rsUserDestroy(RSUser* user);
void rsUserWait(RSUser* user);

#endif
