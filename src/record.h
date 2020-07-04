/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef SR_RECORD_H
#define SR_RECORD_H
#include "encoder/encoder.h"

void srRecordInit(void);
void srRecordExit(void);
const SREncoder* srRecordVideo(void);

#endif
