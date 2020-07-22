/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef RS_ENCODER_VIDEO_H
#define RS_ENCODER_VIDEO_H
#include "encoder.h"

/**
 * Creates a video encoder by trying a few different options to see what works and
 * choosing the best one.
 */
void rsVideoEncoderCreate(RSEncoder *encoder, const RSInput *input);

/**
 * Same as `rsVideoEncoderCreate` but only creates software encoders. Used to fix some
 * hardware encoders which do not support global headers.
 *
 * Unlike most functions this function will return an error code on failure instead of
 * crashing. This is because a hardware encoder can still work even if it cannot create
 * global headers.
 */
int rsVideoEncoderCreateSW(RSEncoder *encoder, const RSInput *input);

#endif
