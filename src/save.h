/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef RS_SAVE_H
#define RS_SAVE_H

/**
 * Initializes the objects needed for saving.
 */
void rsSaveInit(void);

/**
 * Frees up resources used for saving.
 */
void rsSaveExit(void);

/**
 * Saves the currently encoded packets to a file.
 */
void rsSave(void);

#endif
