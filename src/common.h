/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef RS_COMMON_H
#define RS_COMMON_H
#include <rs-build-config.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef RS_CONFIG_ATTR_NORETURN
#define RS_NORETURN __attribute__((noreturn))
#else
#define RS_NORETURN
#endif

#define RS_ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

typedef enum RSErrorFlags { RS_ERROR_SIGNAL_FRAME = 1 } RSErrorFlags;

void rsLog(const char *fmt, ...);
RS_NORETURN void rsError(RSErrorFlags flags, const char *fmt, ...);
void *rsAllocate(size_t size);
void rsFree(void *ptr);
void *rsReallocate(void *ptr, size_t size);

static inline void rsClear(void *ptr, size_t size) {
   memset(ptr, 0, size);
}

#endif
