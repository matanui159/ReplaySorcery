/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef RS_USER_H
#define RS_USER_H

/**
 * A user is an interface out to the user of the computer. It mainly allows waiting for
 * the user to specify to save the file with a keypress, but with different backends
 * including one for debugging. Like a UI but 'UI' is usually referring to something
 * graphical.
 */
typedef struct RSUser {
   /**
    * An internal callback set by the implementation which destroys the user and frees
    * up resources. Can be NULL in which case nothing is done on destruction.
    */
   void (*destroy)(struct RSUser* user);

   /**
    * An internal callback set by the implementation for waiting on input. Can be NULL in
    * which case `rsUserWait` returns immediately.
    */
   void (*wait)(struct RSUser* user);

   /**
    * Extra data the implementation can allocate. Can be NULL.
    */
   void* extra;
} RSUser;

/**
 * Destroys the user object and frees up whatever resources it might be holding onto.
 */
void rsUserDestroy(RSUser* user);

/**
 * Waits for the users input to specify to save a file.
 */
void rsUserWait(RSUser* user);

#endif
