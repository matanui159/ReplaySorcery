/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef SR_ENCODER_H
#define SR_ENCODER_H
#include "../pktcircle.h"
#include "../input/input.h"
#include <libavutil/avutil.h>
#include <libavutil/dict.h>
#include <libavutil/hwcontext.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>

typedef struct SREncoderParams {
   const SRInput* input;
   const char* name;
   enum AVPixelFormat format;
   AVDictionary** options;
   enum AVHWDeviceType hwType;
   enum AVPixelFormat hwFormat;
} SREncoderParams;

typedef struct SREncoder {
   enum AVPixelFormat format;
   AVCodecContext* codecCtx;
   AVRational inputBase;
   struct SwsContext* scaleCtx;
   AVFrame* scaleFrame;
   AVBufferRef* hwDeviceRef;
   AVBufferRef* hwFramesRef;
   AVFrame* hwFrame;
   SRPacketCircle pktCircle;
} SREncoder;

int srEncoderCreate(SREncoder* encoder, const SREncoderParams* params);
void srEncoderDestroy(SREncoder* encoder);
void srEncode(SREncoder* encoder, const AVFrame* frame);

#endif
