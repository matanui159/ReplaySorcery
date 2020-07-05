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

typedef struct RSEncoder {
   AVCodecContext* codecCtx;
   struct SwsContext* scaleCtx;
   AVFrame* scaleFrame;
   AVBufferRef* hwDeviceRef;
   AVBufferRef* hwFramesRef;
   AVFrame* hwFrame;
   RSPacketCircle pktCircle;
} RSEncoder;

int rsEncoderCreate(RSEncoder* encoder, const RSEncoderParams* params);
void rsEncoderDestroy(RSEncoder* encoder);
void rsEncode(RSEncoder* encoder, const AVFrame* frame);

#endif
