/*
 * Copyright (C) 2020  Joshua Minter
 *
 * This file is part of ReplaySorcery.
 *
 * ReplaySorcery is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ReplaySorcery is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ReplaySorcery.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "../config.h"
#include "../main.h"
#include "../util.h"
#include "adevice.h"
#include "rsbuild.h"
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/bprint.h>
#include <libavutil/time.h>

#ifdef RS_BUILD_PULSE_FOUND
#include <pulse/pulseaudio.h>

typedef struct PulseDevice {
   RSDevice device;
   int error;
   char *streamName;
   pa_mainloop *mainloop;
   pa_context *context;
   pa_stream *stream;
} PulseDevice;

static int pulseDeviceError(int error) {
   switch (-error) {
   case PA_OK:
      return 0;
   case PA_ERR_ACCESS:
      return AVERROR(EACCES);
   case PA_ERR_COMMAND:
   case PA_ERR_NOTSUPPORTED:
   case PA_ERR_OBSOLETE:
   case PA_ERR_NOTIMPLEMENTED:
      return AVERROR(ENOSYS);
   case PA_ERR_INVALID:
   case PA_ERR_INVALIDSERVER:
      return AVERROR(EINVAL);
   case PA_ERR_EXIST:
      return AVERROR(EEXIST);
   case PA_ERR_NOENTITY:
      return AVERROR(ENOENT);
   case PA_ERR_CONNECTIONREFUSED:
   case PA_ERR_PROTOCOL:
   case PA_ERR_CONNECTIONTERMINATED:
   case PA_ERR_NODATA:
   case PA_ERR_TOOLARGE:
   case PA_ERR_FORKED:
   case PA_ERR_IO:
      return AVERROR(EIO);
   case PA_ERR_BUSY:
      return AVERROR(EBUSY);
   default:
      return AVERROR_EXTERNAL;
   }
}

static int pulseDeviceIterate(PulseDevice *pulse) {
   int ret;
   if ((ret = pa_mainloop_iterate(pulse->mainloop, 1, NULL)) < 0) {
      av_log(NULL, AV_LOG_ERROR, "Failed to iterate pulse mainloop: %s\n",
             pa_strerror(ret));
      return pulseDeviceError(ret);
   }
   return 0;
}

static int pulseDeviceWait(PulseDevice *pulse, pa_operation *op) {
   int ret;
   if (op == NULL) {
      ret = pa_context_errno(pulse->context);
      av_log(NULL, AV_LOG_ERROR, "Failed to start pulse operation: %s\n",
             pa_strerror(ret));
      ret = pulseDeviceError(ret);
      goto error;
   }

   pa_operation_state_t state;
   while ((state = pa_operation_get_state(op)) == PA_OPERATION_RUNNING) {
      if ((ret = pulseDeviceIterate(pulse)) < 0) {
         goto error;
      }
   }
   if (state != PA_OPERATION_DONE) {
      av_log(NULL, AV_LOG_ERROR, "Failed to run pulse operation\n");
      ret = AVERROR_EXTERNAL;
      goto error;
   }
   if ((ret = pulse->error) < 0) {
      av_log(NULL, AV_LOG_ERROR, "Failed to run pulse operation callback: %s\n",
             av_err2str(ret));
      goto error;
   }

   ret = 0;
error:
   pa_operation_unref(op);
   return ret;
}

static void pulseDeviceDestroy(RSDevice *device) {
   PulseDevice *pulse = (PulseDevice *)device;
   avcodec_parameters_free(&pulse->device.params);
   if (pulse->stream != NULL) {
      if (pa_stream_get_state(pulse->stream) != PA_STREAM_UNCONNECTED) {
         pa_stream_disconnect(pulse->stream);
      }
      pa_stream_unref(pulse->stream);
   }
   av_freep(&pulse->streamName);
   if (pulse->context != NULL) {
      if (pa_context_get_state(pulse->context) != PA_CONTEXT_UNCONNECTED) {
         pa_context_disconnect(pulse->context);
      }
      pa_context_unref(pulse->context);
   }
   if (pulse->mainloop != NULL) {
      pa_mainloop_free(pulse->mainloop);
   }
}

static int pulseDeviceRead(PulseDevice *pulse, AVFrame *frame) {
   int ret;
   const void *data;
   size_t size;
   if ((ret = pa_stream_peek(pulse->stream, &data, &size)) < 0) {
      av_log(NULL, AV_LOG_ERROR, "Failed to read from pulse stream: %s\n",
             pa_strerror(ret));
      return pulseDeviceError(ret);
   }
   if (data == NULL) {
      ret = AVERROR(EAGAIN);
      if (size > 0) {
         av_log(NULL, AV_LOG_WARNING, "%zu byte hole in pulse stream\n", size);
         goto error;
      } else {
         return ret;
      }
   }

   frame->nb_samples = (int)(size / sizeof(float));
   frame->pts = av_gettime_relative() -
                av_rescale(frame->nb_samples, AV_TIME_BASE, rsConfig.audioSamplerate);
   if ((ret = av_frame_get_buffer(frame, 0)) < 0) {
      goto error;
   }
   memcpy(frame->data[0], data, size);

   ret = 0;
error:
   pa_stream_drop(pulse->stream);
   return ret;
}

static int pulseDeviceGetFrame(RSDevice *device, AVFrame *frame) {
   int ret;
   PulseDevice *pulse = (PulseDevice *)device;
   AVCodecParameters *params = pulse->device.params;
   frame->format = params->format;
   frame->sample_rate = params->sample_rate;
   frame->channels = params->channels;
   frame->channel_layout = params->channel_layout;
   while ((ret = pulseDeviceRead(pulse, frame)) == AVERROR(EAGAIN)) {
      if ((ret = pulseDeviceIterate(pulse)) < 0) {
         return ret;
      }
   }
   if (ret < 0) {
      return ret;
   }
   return 0;
}

static void pulseDeviceServerInfo(pa_context *context, const pa_server_info *info,
                                  void *extra) {
   (void)context;
   PulseDevice *pulse = extra;
   av_log(NULL, AV_LOG_INFO, "Pulse server: %s %s\n", info->server_name,
          info->server_version);
   pulse->streamName = rsFormat("%s.monitor", info->default_sink_name);
   if (pulse->streamName == NULL) {
      pulse->error = AVERROR(ENOMEM);
      return;
   }
}
#endif

int rsPulseDeviceCreate(RSDevice **device) {
   int ret;
#ifdef RS_BUILD_PULSE_FOUND
   PulseDevice *pulse = av_mallocz(sizeof(PulseDevice));
   *device = &pulse->device;
   if (pulse == NULL) {
      ret = AVERROR(ENOMEM);
      goto error;
   }

   pulse->device.destroy = pulseDeviceDestroy;
   pulse->device.getFrame = pulseDeviceGetFrame;
   pulse->mainloop = pa_mainloop_new();
   if (pulse->mainloop == NULL) {
      av_log(NULL, AV_LOG_ERROR, "Failed to create pulse mainloop\n");
      ret = AVERROR_EXTERNAL;
      goto error;
   }

   pulse->context = pa_context_new(pa_mainloop_get_api(pulse->mainloop), rsName);
   if (pulse->context == NULL) {
      av_log(NULL, AV_LOG_ERROR, "Failed to create pulse context\n");
      ret = AVERROR_EXTERNAL;
      goto error;
   }
   if ((ret = pa_context_connect(pulse->context, NULL, PA_CONTEXT_NOAUTOSPAWN, NULL)) <
       0) {
      av_log(NULL, AV_LOG_ERROR, "Failed to connect pulse context: %s\n",
             pa_strerror(ret));
      ret = pulseDeviceError(ret);
      goto error;
   }

   pa_context_state_t state;
   while ((state = pa_context_get_state(pulse->context)) != PA_CONTEXT_READY) {
      if (!PA_CONTEXT_IS_GOOD(state)) {
         av_log(NULL, AV_LOG_ERROR, "Failed to setup pulse context\n");
         ret = AVERROR_EXTERNAL;
         goto error;
      }
      if ((ret = pulseDeviceIterate(pulse)) < 0) {
         goto error;
      }
   }

   pa_operation *op =
       pa_context_get_server_info(pulse->context, pulseDeviceServerInfo, pulse);
   if ((ret = pulseDeviceWait(pulse, op)) < 0) {
      av_log(NULL, AV_LOG_ERROR, "Failed to get pulse server info: %s\n",
             av_err2str(ret));
      goto error;
   }

   av_log(NULL, AV_LOG_INFO, "Pulse stream name: %s\n", pulse->streamName);
   pulse->stream = pa_stream_new(pulse->context, rsName,
                                 &(pa_sample_spec){
                                     .format = PA_SAMPLE_FLOAT32NE,
                                     .rate = (uint32_t)rsConfig.audioSamplerate,
                                     .channels = 1,
                                 },
                                 NULL);
   if (pulse->stream == NULL) {
      ret = pa_context_errno(pulse->context);
      av_log(NULL, AV_LOG_ERROR, "Failed to create pulse stream: %s\n", pa_strerror(ret));
      ret = pulseDeviceError(ret);
      goto error;
   }
   if ((ret = pa_stream_connect_record(pulse->stream, pulse->streamName, NULL, 0)) < 0) {
      av_log(NULL, AV_LOG_ERROR, "Failed to connect pulse stream: %s\n",
             pa_strerror(ret));
      ret = pulseDeviceError(ret);
      goto error;
   }

   pa_stream_state_t streamState;
   while ((streamState = pa_stream_get_state(pulse->stream)) == PA_STREAM_CREATING) {
      if ((ret = pulseDeviceIterate(pulse)) < 0) {
         goto error;
      }
   }
   if (streamState != PA_STREAM_READY) {
      av_log(NULL, AV_LOG_ERROR, "Failed to create pulse stream\n");
      ret = AVERROR_EXTERNAL;
      goto error;
   }

   AVCodecParameters *params = avcodec_parameters_alloc();
   params = avcodec_parameters_alloc();
   pulse->device.params = params;
   if (params == NULL) {
      ret = AVERROR(ENOMEM);
      goto error;
   }
   params->codec_type = AVMEDIA_TYPE_AUDIO;
   params->format = AV_SAMPLE_FMT_FLTP;
   params->sample_rate = rsConfig.audioSamplerate;
   params->channels = 1;
   params->channel_layout = AV_CH_LAYOUT_MONO;

   return 0;

#else
   (void)device;
   av_log(NULL, AV_LOG_ERROR, "Pulse was not found during compilation\n");
   ret = AVERROR(ENOSYS);
   goto error;
#endif
error:
   rsDeviceDestroy(device);
   return ret;
}
