/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef RS_CONFIG_FILE_H
#define RS_CONFIG_FILE_H

/**
 * Reads a config file if it exists and loads its values into the global program config.
 */
void rsConfigFileRead(const char* path);

#endif
