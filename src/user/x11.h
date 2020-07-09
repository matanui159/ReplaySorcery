/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef RS_USER_X11_H
#define RS_USER_X11_H
#include "user.h"

/**
 * Creates a user that listens to X11 inputs for a keypress.
 */
int rsX11UserCreate(RSUser* user);

#endif
