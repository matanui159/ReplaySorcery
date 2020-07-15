/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef RS_UTIL_MEMORY_H
#define RS_UTIL_MEMORY_H
#include "../std.h"

void *rsMemoryCreate(size_t size);
void rsMemoryDestroy(void *mem);
void *rsMemoryResize(void *mem, size_t size);

static inline void rsMemoryClear(void *mem, size_t size) {
   memset(mem, 0, size);
}

#endif
