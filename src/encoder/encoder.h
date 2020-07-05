/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef RS_ENCODER_H
#define RS_ENCODER_H
#include "../input/input.h"
#include "../pktcircle.h"
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/dict.h>
#include <libavutil/hwcontext.h>
#include <libswscale/swscale.h>

/**
 * Parameters for creating the encoder. This was split out into a struct since there is
 * alot of them and some of them are optional.
 */
typedef struct RSEncoderParams {
   /**
    * A pointer to the input where the frames are going to come from.
    */
   const RSInput* input;

   /**
    * The name of the encoder to use.
    */
   const char* name;

   /**
    * The pixel format of the encoder. For hardware encoders this is the software format
    * and the hardware format is listed below.
    */
   enum AVPixelFormat format;

   /**
    * Options to pass to the encoder creation.
    */
   AVDictionary** options;

   /**
    * The hardware device type for hardware encoders. Set this to `AV_HWDEVICE_TYPE_NONE`
    * for software encoders or hardware encoders that do not require a device.
    */
   enum AVHWDeviceType hwType;

   /**
    * The hardware format for hardware encoders.
    */
   enum AVPixelFormat hwFormat;
} RSEncoderParams;

/**
 * An output encoder. Encoded packets are written into its packet circle.
 */
typedef struct RSEncoder {
   /**
    * The codec context of the encoder.
    */
   AVCodecContext* codecCtx;

   /**
    * A scale context for converting frames. Only created if the provided input has a
    * different format or resolution to the encoder.
    */
   struct SwsContext* scaleCtx;

   /**
    * A preallocated buffer frame to put scaling results into. Only created if `scaleCtx`
    * is created.
    */
   AVFrame* scaleFrame;

   /**
    * A reference to a hardware device used for encoding. Only created if this is a
    * hardware-based encoder.
    */
   AVBufferRef* hwDeviceRef;

   /**
    * A reference to a hardware frame pool. Only has a size of 1 to get `hwFrame` out of
    * below. Only created if `hwDeviceRef` is created.
    */
   AVBufferRef* hwFramesRef;

   /**
    * A buffer frame for uploading data to hardware. Created from `hwFramesRef`.
    */
   AVFrame* hwFrame;

   /**
    * The packet circle that encoded packets are written to. Read from this to get the
    * encoded data. See `pktcircle.h` on how to properly read from a packet circle.
    */
   RSPacketCircle pktCircle;
} RSEncoder;

/**
 * Creates a new encoder with the given parameters. Note that unlike most functions this
 * returns an error code on failure instead of crashing. This makes it possible to test
 * multiple encoders and find one that works.
 */
int rsEncoderCreate(RSEncoder* encoder, const RSEncoderParams* params);

/**
 * Destroys the provided encoder.
 */
void rsEncoderDestroy(RSEncoder* encoder);

/**
 * Encodes a single frame into the packet circle. Does not modify or destroy the provided
 * frame.
 */
void rsEncode(RSEncoder* encoder, const AVFrame* frame);

#endif
