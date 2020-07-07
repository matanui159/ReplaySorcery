/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef RS_PATH_H
#define RS_PATH_H
#include <stddef.h>

/**
 * A dynamically resizing file path.
 */
typedef struct RSPath {
   /**
    * The current NULL-terminated string value of the path.
    */
   char* value;

   /**
    * The size of the above value not including the NULL-terminator.
    */
   size_t size;

   /**
    * The maximum capacity of `value`. Used internally to reduce the amount of
    * reallocations.
    */
   size_t capacity;
} RSPath;

/**
 * Creates a new path with the initial value from `str`.
 */
void rsPathCreate(RSPath* path, const char* str);

/**
 * The destroys the path and frees all memory used by it.
 */
void rsPathDestroy(RSPath* path);

/**
 * Concatenates another path to the provided one, making sure there is a `/` inbetween.
 * If the path to be added starts with `/` or `~`, it is treated as an absolute path.
 */
void rsPathAdd(RSPath* path, const char* str);

/**
 * Same as `rsAddPath`, but the string can have `strftime` formatting parameters.
 */
void rsPathAddDated(RSPath* path, const char* str);

#endif
