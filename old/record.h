/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef RS_RECORD_H
#define RS_RECORD_H
#include "encoder/encoder.h"

void rsRecordInit(void);
void rsRecordExit(void);
const RSEncoder *rsRecordVideo(void);

#endif
