/*
 * Copyright (C) 2020  Joshua Minter
 *
 * This file is part of ReplaySorcery.
 *
 * ReplaySorcery is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ReplaySorcery is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ReplaySorcery.  If not, see <https://www.gnu.org/licenses/>.
 */

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
