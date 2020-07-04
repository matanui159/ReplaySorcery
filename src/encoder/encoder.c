#include "encoder.h"
#include "../error.h"

static int encoderHardwareCreate(SREncoder* encoder, const SREncoderParams* params, AVCodecContext* inputCodec) {
   int ret;
   if ((ret = av_hwdevice_ctx_create(&encoder->hwDeviceRef, params->hwType, NULL, NULL, 0)) < 0) {
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
   srCheck(av_hwframe_get_buffer(encoder->hwFramesRef, encoder->hwFrame, 0));
   return 0;
}

static void encoderHardwareDestroy(SREncoder* encoder) {
   if (encoder->hwDeviceRef != NULL) {
      av_frame_free(&encoder->hwFrame);
      av_buffer_unref(&encoder->hwFramesRef);
      av_buffer_unref(&encoder->hwDeviceRef);
   }
}

int srEncoderCreate(SREncoder* encoder, const SREncoderParams* params) {
   int ret;
   encoder->format = params->format;
   encoder->inputBase = params->input->formatCtx->streams[0]->time_base;
   AVCodecContext* inputCodec = params->input->codecCtx;
   AVCodec* codec = avcodec_find_encoder_by_name(params->name);
   if (codec == NULL) {
      av_dict_free(params->options);
      return AVERROR_ENCODER_NOT_FOUND;
   }

   if (params->hwType != AV_HWDEVICE_TYPE_NONE) {
      if ((ret = encoderHardwareCreate(encoder, params, inputCodec)) < 0) {
         av_dict_free(params->options);
         return ret;
      }
   }

   AVCodecContext* outputCodec = avcodec_alloc_context3(codec);
   encoder->codecCtx = outputCodec;
   outputCodec->width = inputCodec->width;
   outputCodec->height = inputCodec->height;
   outputCodec->pix_fmt = encoder->format;
   outputCodec->time_base = AV_TIME_BASE_Q;
   outputCodec->gop_size = 0;
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

   if (encoder->format != inputCodec->pix_fmt) {
      encoder->scaleCtx = sws_getContext(
         inputCodec->width, inputCodec->height, inputCodec->pix_fmt,
         outputCodec->width, outputCodec->height, encoder->format,
         SWS_FAST_BILINEAR, NULL, NULL, NULL
      );
      if (encoder->scaleCtx == NULL) srError(AVERROR_EXTERNAL);

      encoder->scaleFrame = av_frame_alloc();
      encoder->scaleFrame->width = outputCodec->width;
      encoder->scaleFrame->height = outputCodec->height;
      encoder->scaleFrame->format = encoder->format;
      srCheck(av_frame_get_buffer(encoder->scaleFrame, 0));
   }

   srPacketCircleCreate(&encoder->pktCircle);
   return 0;
}

void srEncoderDestroy(SREncoder* encoder) {
   srPacketCircleDestroy(&encoder->pktCircle);
   if (encoder->scaleCtx != NULL) {
      av_frame_free(&encoder->hwFrame);
      sws_freeContext(encoder->scaleCtx);
   }
   avcodec_free_context(&encoder->codecCtx);
   encoderHardwareDestroy(encoder);
}

void srEncode(SREncoder* encoder, const AVFrame* frame) {
   AVFrame* inputFrame = (AVFrame*)frame;
   if (encoder->scaleCtx != NULL) {
      sws_scale(
         encoder->scaleCtx,
         (const uint8_t* const*)inputFrame->data, inputFrame->linesize,
         0, inputFrame->height,
         encoder->scaleFrame->data, encoder->scaleFrame->linesize
      );
      inputFrame = encoder->scaleFrame;
   }

   if (encoder->hwDeviceRef != NULL) {
      srCheck(av_hwframe_transfer_data(encoder->hwFrame, inputFrame, 0));
      inputFrame = encoder->hwFrame;
   }

   if (inputFrame != frame) {
      srCheck(av_frame_copy_props(inputFrame, frame));
   }
   inputFrame->pts = av_rescale_q(inputFrame->pts, encoder->inputBase, encoder->codecCtx->time_base);

   srCheck(avcodec_send_frame(encoder->codecCtx, inputFrame));
   int ret = avcodec_receive_packet(encoder->codecCtx, encoder->pktCircle.input);
   if (ret == AVERROR(EAGAIN)) return;
   srCheck(ret);
   if (!(encoder->pktCircle.input->flags & AV_PKT_FLAG_KEY)) {
      av_log(NULL, AV_LOG_WARNING, "Encoded packet is not a keyframe\n");
   }
   srPacketCircleRotate(&encoder->pktCircle);
}
