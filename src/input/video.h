/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef SR_INPUT_VIDEO_H
#define SR_INPUT_VIDEO_H
#include "input.h"

/**
 * Checks different supported video inputs and finds one that works.
 */
void srVideoInputCreate(SRInput* input);

#endif
