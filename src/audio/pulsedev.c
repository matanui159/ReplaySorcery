/*
 * Copyright (C) 2020-2021  Joshua Minter
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
   int error;
   pa_mainloop *mainloop;
   pa_context *context;
   pa_stream *stream;
   char *sink;
   int serverChanged;
} PulseDevice;

static int pulseDeviceError(int error) {
   switch (FFABS(error)) {
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
      av_log(NULL, AV_LOG_ERROR, "Failed to iterate PulseAudio mainloop: %s\n",
             pa_strerror(ret));
      return pulseDeviceError(ret);
   }
   return 0;
}

static int pulseDeviceWait(PulseDevice *pulse, pa_operation *op) {
   int ret;
   if (op == NULL) {
      ret = pa_context_errno(pulse->context);
      av_log(NULL, AV_LOG_ERROR, "Failed to start PulseAudio operation: %s\n",
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
      av_log(NULL, AV_LOG_ERROR, "Failed to run PulseAudio operation\n");
      ret = AVERROR_EXTERNAL;
      goto error;
   }
   if ((ret = pulse->error) < 0) {
      pulse->error = 0;
      if (ret != AVERROR(EAGAIN)) {
         av_log(NULL, AV_LOG_ERROR, "Failed to run PulseAudio operation callback: %s\n",
                av_err2str(ret));
      }
      goto error;
   }

   ret = 0;
error:
   pa_operation_unref(op);
   return ret;
}

static void pulseDeviceStreamDestroy(PulseDevice *pulse) {
   if (pulse->stream != NULL) {
      if (pa_stream_get_state(pulse->stream) != PA_STREAM_UNCONNECTED) {
         pa_stream_disconnect(pulse->stream);
      }
      pa_stream_unref(pulse->stream);
   }
}

static int pulseDeviceStreamCreate(PulseDevice *pulse, const char *name) {
   int ret;
   pulseDeviceStreamDestroy(pulse);
   av_log(NULL, AV_LOG_INFO, "Connecting to Pulse Audio device: %s...\n",
          name == NULL ? "system" : name);
   pulse->stream = pa_stream_new(pulse->context, RS_NAME,
                                 &(pa_sample_spec){
                                     .format = PA_SAMPLE_FLOAT32NE,
                                     .channels = 1,
                                     .rate = (uint32_t)rsConfig.audioSamplerate,
                                 },
                                 NULL);
   if (pulse->stream == NULL) {
      ret = pa_context_errno(pulse->context);
      av_log(NULL, AV_LOG_ERROR, "Failed to create PulseAudio stream: %s\n",
             pa_strerror(ret));
      return pulseDeviceError(ret);
   }
   if ((ret = pa_stream_connect_record(pulse->stream, name, NULL, 0)) < 0) {
      av_log(NULL, AV_LOG_ERROR, "Failed to connect PulseAudio stream: %s\n",
             pa_strerror(ret));
      return pulseDeviceError(ret);
   }

   pa_stream_state_t state;
   while ((state = pa_stream_get_state(pulse->stream)) == PA_STREAM_CREATING) {
      if ((ret = pulseDeviceIterate(pulse)) < 0) {
         return ret;
      }
   }
   if (state != PA_STREAM_READY) {
      av_log(NULL, AV_LOG_ERROR, "Failed to setup PulseAudio stream\n");
      return AVERROR_EXTERNAL;
   }
   return 0;
}

static void pulseDeviceServerInfo(pa_context *context, const pa_server_info *info,
                                  void *extra) {
   int ret;
   (void)context;
   PulseDevice *pulse = extra;
   if (pulse->sink == NULL) {
      av_log(NULL, AV_LOG_INFO, "PulseAudio server: %s %s\n", info->server_name,
             info->server_version);
   }
   if (pulse->sink != NULL && strcmp(pulse->sink, info->default_sink_name) == 0) {
      pulse->error = AVERROR(EAGAIN);
      return;
   }

   av_freep(&pulse->sink);
   pulse->sink = av_strdup(info->default_sink_name);
   if (pulse->sink == NULL) {
      ret = AVERROR(ENOMEM);
      goto error;
   }

   return;
error:
   av_log(NULL, AV_LOG_ERROR, "Failed to handle PulseAudio server info: %s\n",
          av_err2str(ret));
   pulse->error = ret;
}

static int pulseDeviceStreamAuto(PulseDevice *pulse) {
   int ret;
   pa_operation *op =
       pa_context_get_server_info(pulse->context, pulseDeviceServerInfo, pulse);
   if ((ret = pulseDeviceWait(pulse, op)) < 0) {
      return ret;
   }

   char *source = rsFormat("%s.monitor", pulse->sink);
   if (source == NULL) {
      return AVERROR(ENOMEM);
   }
   ret = pulseDeviceStreamCreate(pulse, source);
   av_freep(&source);
   if (ret < 0) {
      return ret;
   }
   return 0;
}

static void pulseDeviceServerChange(pa_context *context,
                                    pa_subscription_event_type_t type, uint32_t index,
                                    void *extra) {
   (void)context;
   (void)type;
   (void)index;
   PulseDevice *pulse = extra;
   pulse->serverChanged = 1;
}

static void pulseDeviceDestroy(RSDevice *device) {
   PulseDevice *pulse = device->extra;
   if (pulse != NULL) {
      av_freep(&pulse->sink);
      pulseDeviceStreamDestroy(pulse);
      if (pulse->context != NULL) {
         if (pa_context_get_state(pulse->context) != PA_CONTEXT_UNCONNECTED) {
            pa_context_disconnect(pulse->context);
         }
         pa_context_unref(pulse->context);
         pulse->context = NULL;
      }
      if (pulse->mainloop != NULL) {
         pa_mainloop_free(pulse->mainloop);
         pulse->mainloop = NULL;
      }
      av_freep(&device->extra);
   }
}

static int pulseDeviceRead(PulseDevice *pulse, AVFrame *frame) {
   int ret;
   if (pulse->serverChanged) {
      pulse->serverChanged = 0;
      if ((ret = pulseDeviceStreamAuto(pulse)) < 0) {
         return ret;
      }
   }

   const void *data;
   size_t size;
   if ((ret = pa_stream_peek(pulse->stream, &data, &size)) < 0) {
      av_log(NULL, AV_LOG_ERROR, "Failed to read from PulseAudio stream: %s\n",
             pa_strerror(ret));
      return pulseDeviceError(ret);
   }
   if (size == 0) {
      return AVERROR(EAGAIN);
   }

   frame->nb_samples = (int)(size / sizeof(float));
   frame->pts = av_rescale(av_gettime_relative(), frame->sample_rate, AV_TIME_BASE) -
                frame->nb_samples;
   if ((ret = av_frame_get_buffer(frame, 0)) < 0) {
      goto error;
   }
   if (data == NULL) {
      av_log(NULL, AV_LOG_WARNING, "%zu byte hole in PulseAudio stream\n", size);
      rsClear(frame->data[0], size);
   } else {
      memcpy(frame->data[0], data, size);
   }

   ret = 0;
error:
   pa_stream_drop(pulse->stream);
   return ret;
}

static int pulseDeviceNextFrame(RSDevice *device, AVFrame *frame) {
   int ret;
   PulseDevice *pulse = device->extra;
   frame->format = device->params->format;
   frame->channels = device->params->channels;
   frame->channel_layout = device->params->channel_layout;
   frame->sample_rate = device->params->sample_rate;
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
#endif

int rsPulseDeviceCreate(RSDevice *device) {
#ifdef RS_BUILD_PULSE_FOUND
   int ret;
   if ((ret = rsDeviceCreate(device)) < 0) {
      goto error;
   }

   PulseDevice *pulse = av_mallocz(sizeof(PulseDevice));
   device->extra = pulse;
   device->destroy = pulseDeviceDestroy;
   device->nextFrame = pulseDeviceNextFrame;
   device->params->format = AV_SAMPLE_FMT_FLT;
   device->params->channels = 1;
   device->params->channel_layout = AV_CH_LAYOUT_MONO;
   device->params->sample_rate = rsConfig.audioSamplerate;
   if (pulse == NULL) {
      ret = AVERROR(ENOMEM);
      goto error;
   }

   pulse->mainloop = pa_mainloop_new();
   if (pulse->mainloop == NULL) {
      av_log(NULL, AV_LOG_ERROR, "Failed to create PulseAudio mainloop\n");
      ret = AVERROR_EXTERNAL;
      goto error;
   }

   pulse->context = pa_context_new(pa_mainloop_get_api(pulse->mainloop), RS_NAME);
   if (pulse->context == NULL) {
      av_log(NULL, AV_LOG_ERROR, "Failed to create PulseAudio context\n");
      ret = AVERROR_EXTERNAL;
      goto error;
   }
   if ((ret = pa_context_connect(pulse->context, NULL, PA_CONTEXT_NOAUTOSPAWN, NULL)) <
       0) {
      av_log(NULL, AV_LOG_ERROR, "Failed to connect PulseAudio context: %s\n",
             pa_strerror(ret));
      ret = pulseDeviceError(ret);
      goto error;
   }

   pa_context_state_t state;
   while ((state = pa_context_get_state(pulse->context)) != PA_CONTEXT_READY) {
      if (!PA_CONTEXT_IS_GOOD(state)) {
         av_log(NULL, AV_LOG_ERROR, "Failed to setup PulseAudio context\n");
         ret = AVERROR_EXTERNAL;
         goto error;
      }
      if ((ret = pulseDeviceIterate(pulse)) < 0) {
         goto error;
      }
   }

   if (strcmp(rsConfig.audioDevice, "auto") == 0) {
      if ((ret = pulseDeviceStreamAuto(pulse)) < 0) {
         goto error;
      }
      pa_context_set_subscribe_callback(pulse->context, pulseDeviceServerChange, pulse);
      pa_operation *op = pa_context_subscribe(
          pulse->context, PA_SUBSCRIPTION_EVENT_SERVER | PA_SUBSCRIPTION_EVENT_CHANGE,
          NULL, NULL);
      if ((ret = pulseDeviceWait(pulse, op)) < 0) {
         goto error;
      }
   } else if (strcmp(rsConfig.audioDevice, "system") == 0) {
      if ((ret = pulseDeviceStreamCreate(pulse, NULL)) < 0) {
         goto error;
      }
   } else {
      if ((ret = pulseDeviceStreamCreate(pulse, rsConfig.audioDevice)) < 0) {
         goto error;
      }
   }

   return 0;
error:
   rsDeviceDestroy(device);
   return ret;

#else
   (void)device;
   av_log(NULL, AV_LOG_ERROR, "Pulse was not found during compilation\n");
   return AVERROR(ENOSYS);
#endif
}
