/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef RS_INPUT_H
#define RS_INPUT_H
#include <libavutil/avutil.h>
#include <libavutil/dict.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>

/**
 * A single stream A/V input device.
 */
typedef struct RSInput {
   /**
    * The format context of the input device.
    */
   AVFormatContext* formatCtx;

   /**
    * The codec context to decode the input packets. This is usually `rawvideo`.
    */
   AVCodecContext* codecCtx;

   /**
    * A buffer packet to read into and decode from.
    */
   AVPacket* packet;
} RSInput;

/**
 * Initializes global state for creating inputs.
 */
void rsInputInit(void);

/**
 * Creates a new input. Returns 0 on success or a negative error code on failure. Unlike
 * most functions this will return safely on failure rather than crashing. This is so it
 * is possible to test multiple inputs and find one that works.
 *
 * `name` is the name of the input device.
 * `url` is a device-dependent descriptor (for X11 it is the screen).
 * `options` is a dictionary of options to pass to the device creation. This will be freed
 * before this function returns, even if it failed.
 */
int rsInputCreate(RSInput* input, const char* name, const char* url, AVDictionary** options);

/**
 * Destroys the input device.
 */
void rsInputDestroy(RSInput* input);

/**
 * Reads a frame from the input device.
 */
void rsInputRead(RSInput* input, AVFrame* frame);

#endif
