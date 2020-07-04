/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "video.h"
#include "../error.h"
#include "sr-config.h"
#include <libavutil/avutil.h>
#include <libavutil/dict.h>

void srVideoEncoderCreate(SREncoder* encoder, const SRInput* input) {
   int ret;
   AVDictionary* options = NULL;
   av_dict_set(&options, "preset", "fast", 0);
   if ((ret = srEncoderCreate(encoder, &(SREncoderParams){
      .input = input,
      .name = "h264_nvenc",
      .format = AV_PIX_FMT_YUV420P,
      .options = &options
   })) >= 0) {
      return;
   }
   av_log(NULL, AV_LOG_WARNING, "Failed to create nVidia encoder: %s\n", av_err2str(ret));

   av_dict_set(&options, "global_quality", "30", 0);
   if ((ret = srEncoderCreate(encoder, &(SREncoderParams){
      .input = input,
      .name = "h264_vaapi",
      .format = AV_PIX_FMT_NV12,
      .options = &options,
      .hwType = AV_HWDEVICE_TYPE_VAAPI,
      .hwFormat = AV_PIX_FMT_VAAPI
   })) >= 0) {
      return;
   }
   av_log(NULL, AV_LOG_WARNING, "Failed to create VAAPI encoder: %s\n", av_err2str(ret));

#ifdef SR_CONFIG_X264
   av_dict_set(&options, "preset", "ultrafast", 0);
   if ((ret = srEncoderCreate(encoder, &(SREncoderParams){
      .input = input,
      .name = "libx264",
      .format = AV_PIX_FMT_YUV420P,
      .options = &options
   })) >= 0) {
      return;
   }
   av_log(NULL, AV_LOG_WARNING, "Failed to create x264 encoder: %s\n", av_err2str(ret));
#else
   av_log(NULL, AV_LOG_WARNING, "Compile with GPL-licensed x264 support for software encoding\n");
#endif

   srError(AVERROR(ENOSYS));
}
