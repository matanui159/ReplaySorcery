/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef RS_USER_DEBUG_H
#define RS_USER_DEBUG_H
#include "user.h"

/**
 * Creates a debug user that waits for a newline from standard input. Always returns 0
 * but has a return code regardless just to be consistent with other implementations.
 */
int rsDebugUserCreate(RSUser *user);

#endif
