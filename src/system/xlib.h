/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef RS_SYSTEM_XCB_H
#define RS_SYSTEM_XCB_H
#include "../common.h"
#include "../config/config.h"
#include "system.h"

bool rsXlibSystemCreate(RSSystem *system, const RSConfig *config);

#endif
