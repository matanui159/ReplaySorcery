/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "encoder.h"
#include "../error.h"
#include "video.h"
#include <string.h>

/**
 * A very small utility function to try and notify the user of potential encoding issues.
 */
static inline void encoderLogIssues(void) {
   av_log(NULL, AV_LOG_WARNING, ">>> This may cause issues in the output file! <<<\n");
}

static int encoderHardwareCreate(RSEncoder* encoder, const RSEncoderParams* params,
                                 AVCodecContext* inputCodec) {
   int ret;
   if ((ret = av_hwdevice_ctx_create(&encoder->hwDeviceRef, params->hwType, NULL, NULL,
                                     0)) < 0) {
      return ret;
   }

   encoder->hwFramesRef = av_hwframe_ctx_alloc(encoder->hwDeviceRef);
   AVHWFramesContext* hwFramesCtx = (AVHWFramesContext*)encoder->hwFramesRef->data;
   hwFramesCtx->width = inputCodec->width;
   hwFramesCtx->height = inputCodec->height;
   hwFramesCtx->format = params->hwFormat;
   hwFramesCtx->sw_format = params->format;
   hwFramesCtx->initial_pool_size = 1;
   if ((ret = av_hwframe_ctx_init(encoder->hwFramesRef)) < 0) {
      av_buffer_unref(&encoder->hwFramesRef);
      av_buffer_unref(&encoder->hwDeviceRef);
      return ret;
   }

   encoder->hwFrame = av_frame_alloc();
   rsCheck(av_hwframe_get_buffer(encoder->hwFramesRef, encoder->hwFrame, 0));
   return 0;
}

static void encoderHardwareDestroy(RSEncoder* encoder) {
   if (encoder->hwDeviceRef != NULL) {
      av_frame_free(&encoder->hwFrame);
      av_buffer_unref(&encoder->hwFramesRef);
      av_buffer_unref(&encoder->hwDeviceRef);
   }
}

int rsEncoderCreate(RSEncoder* encoder, const RSEncoderParams* params) {
   int ret;
   AVCodecContext* inputCodec = params->input->codecCtx;
   AVCodec* codec = avcodec_find_encoder_by_name(params->name);
   if (codec == NULL) {
      av_dict_free(params->options);
      return AVERROR_ENCODER_NOT_FOUND;
   }

   if (params->hwType == AV_HWDEVICE_TYPE_NONE) {
      encoder->hwDeviceRef = NULL;
      encoder->hwFramesRef = NULL;
   } else {
      if ((ret = encoderHardwareCreate(encoder, params, inputCodec)) < 0) {
         av_dict_free(params->options);
         return ret;
      }
   }

   AVCodecContext* outputCodec = avcodec_alloc_context3(codec);
   encoder->codecCtx = outputCodec;
   outputCodec->width = inputCodec->width;
   outputCodec->height = inputCodec->height;
   outputCodec->pix_fmt = params->format;
   outputCodec->time_base = params->input->formatCtx->streams[0]->time_base;
   outputCodec->gop_size = 0;
   outputCodec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
   if (encoder->hwDeviceRef != NULL) {
      outputCodec->pix_fmt = params->hwFormat;
      outputCodec->hw_device_ctx = av_buffer_ref(encoder->hwDeviceRef);
      outputCodec->hw_frames_ctx = av_buffer_ref(encoder->hwFramesRef);
   }
   ret = avcodec_open2(outputCodec, codec, params->options);
   av_dict_free(params->options);
   if (ret < 0) {
      avcodec_free_context(&encoder->codecCtx);
      encoderHardwareDestroy(encoder);
      return ret;
   }

   if (params->format == inputCodec->pix_fmt) {
      encoder->scaleCtx = NULL;
   } else {
      encoder->scaleCtx = sws_getContext(
          inputCodec->width, inputCodec->height, inputCodec->pix_fmt, outputCodec->width,
          outputCodec->height, params->format, SWS_FAST_BILINEAR, NULL, NULL, NULL);
      if (encoder->scaleCtx == NULL) rsError(AVERROR_EXTERNAL);

      encoder->scaleFrame = av_frame_alloc();
      encoder->scaleFrame->width = outputCodec->width;
      encoder->scaleFrame->height = outputCodec->height;
      encoder->scaleFrame->format = params->format;
      rsCheck(av_frame_get_buffer(encoder->scaleFrame, 0));
   }

   // Some hardware encoder do not support creating global headers. So we instead create
   // a temporary software encoder with similar parameters and try using its global
   // header. It is a bit of a hack, but from my testing it seems to work.
   // TODO: use OpenH264 as a reference and make a custom global header generator.
   if (encoder->hwDeviceRef != NULL && encoder->codecCtx->extradata == NULL) {
      av_log(NULL, AV_LOG_WARNING, "Encoder is missing global header\n");
      av_log(NULL, AV_LOG_INFO,
             "Creating software encoder to copy global header from...\n");
      RSEncoder swEncoder;
      if ((ret = rsVideoEncoderCreateSW(&swEncoder, params->input)) < 0) {
         av_log(NULL, AV_LOG_WARNING,
                "Failed to create software encoder to copy global header: %s\n",
                av_err2str(ret));
         encoderLogIssues();
      } else {
         // We cast to `size_t` since the two function calls both require an argument of
         // type `size_t` and we have `-Wconversion` enabled.
         size_t size = (size_t)swEncoder.codecCtx->extradata_size;
         encoder->codecCtx->extradata_size = (int)size;
         encoder->codecCtx->extradata = av_mallocz(size + AV_INPUT_BUFFER_PADDING_SIZE);
         memcpy(encoder->codecCtx->extradata, swEncoder.codecCtx->extradata, size);
         rsEncoderDestroy(&swEncoder);
         av_log(NULL, AV_LOG_INFO, "Copied %zu-byte global header\n", size);
      }
   }

   rsPacketCircleCreate(&encoder->pktCircle);
   return 0;
}

void rsEncoderDestroy(RSEncoder* encoder) {
   rsPacketCircleDestroy(&encoder->pktCircle);
   if (encoder->scaleCtx != NULL) {
      av_frame_free(&encoder->scaleFrame);
      sws_freeContext(encoder->scaleCtx);
   }
   avcodec_free_context(&encoder->codecCtx);
   encoderHardwareDestroy(encoder);
}

void rsEncode(RSEncoder* encoder, const AVFrame* frame) {
   const AVFrame* inputFrame = frame;
   if (encoder->scaleCtx != NULL) {
      sws_scale(encoder->scaleCtx, (const uint8_t* const*)inputFrame->data,
                inputFrame->linesize, 0, inputFrame->height, encoder->scaleFrame->data,
                encoder->scaleFrame->linesize);
      inputFrame = encoder->scaleFrame;
   }

   if (encoder->hwDeviceRef != NULL) {
      rsCheck(av_hwframe_transfer_data(encoder->hwFrame, inputFrame, 0));
      inputFrame = encoder->hwFrame;
   }

   if (inputFrame != frame) {
      rsCheck(av_frame_copy_props((AVFrame*)inputFrame, frame));
   }
   rsCheck(avcodec_send_frame(encoder->codecCtx, inputFrame));
   int ret = avcodec_receive_packet(encoder->codecCtx, encoder->pktCircle.input);
   if (ret == AVERROR(EAGAIN)) return;
   rsCheck(ret);
   if (!(encoder->pktCircle.input->flags & AV_PKT_FLAG_KEY)) {
      av_log(NULL, AV_LOG_WARNING, "Encoded packet is not a keyframe\n");
      encoderLogIssues();
   }
   rsPacketCircleRotate(&encoder->pktCircle);
}
