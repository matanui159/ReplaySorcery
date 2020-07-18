/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef RS_CONFIG_H
#define RS_CONFIG_H
#include "std.h"

typedef struct RSConfig {
   int offsetX;
   int offsetY;
   int width;
   int height;
   int framerate;
   int duration;
   int compressQuality;
} RSConfig;

void rsConfigLoad(RSConfig *config);

#endif
