/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "memory.h"
#include "log.h"

static void memoryCheck(void *mem, size_t size) {
   if (mem == NULL) {
      rsError("Out of memory when allocating %zu bytes", size);
   }
}

void *rsMemoryCreate(size_t size) {
   if (size == 0) {
      return NULL;
   }
   void *mem = malloc(size);
   memoryCheck(mem, size);
   return mem;
}

void rsMemoryDestroy(void *mem) {
   free(mem);
}

void *rsMemoryResize(void *mem, size_t size) {
   if (size == 0) {
      rsMemoryDestroy(mem);
      return NULL;
   }
   mem = realloc(mem, size);
   memoryCheck(mem, size);
   return mem;
}
