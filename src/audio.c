/*
 * Copyright (C) 2020  Joshua Minter & Patryk Seregiet
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

#include "audio.h"
#include "util/log.h"
#include "memory.h"

static void rsAudioPrepareEncoder(RSAudio *audio, const RSConfig* config);
static void rsAudioPrepareEncoder(RSAudio *audio, const RSConfig* config);
static void rsAudioEncoderPrepareFrame(RSAudioEncoder *audioenc);

void rsAudioCallback(void *userdata, uint8_t *stream, int len) {
	RSAudio* audio = (RSAudio*)userdata;
	size_t end = audio->index + len;
	if (end <= audio->size) {
		memcpy(audio->data + audio->index, stream, len);
		audio->index += len;
		return;
	}

	int diff = end - audio->size;
	int rest = len - diff;
	memcpy(audio->data + audio->index, stream, diff);
	memcpy(audio->data, stream+diff, rest);
	audio->index += rest;
}

int rsAudioCreate(RSAudio *audio, const RSConfig *config) {
   int error;
   if (SDL_Init(SDL_INIT_AUDIO) < 0) {
   	rsError("SDL2 could not initialize! SDL Error: %s", SDL_GetError());
   }
   int devices = SDL_GetNumAudioDevices(1);
   if (devices < 1) {
	rsError("Unable to get audio capture device! SDL Error: %s", SDL_GetError());
   }
   rsLog("Audio driver : %s", SDL_GetCurrentAudioDriver());
   for (int i = 0; i < devices; i++) {
	rsLog("Audio device : %s", SDL_GetAudioDeviceName(i, 1));
   }

   SDL_AudioDeviceID recordingDeviceId = 0;
   
   SDL_AudioSpec ispec;
   SDL_AudioSpec ospec;
   SDL_zero(ispec);
   ispec.freq = config->audioSamplerate;
   ispec.format = AUDIO_S16LSB;
   ispec.channels = config->audioChannels;
   ispec.samples = 4096;
   ispec.callback = rsAudioCallback;
   ispec.userdata = audio;
   //Open recording device
   recordingDeviceId = SDL_OpenAudioDevice(SDL_GetAudioDeviceName(1, SDL_TRUE), SDL_TRUE, 
	   &ispec, &ospec, SDL_AUDIO_ALLOW_FORMAT_CHANGE );
										
   if( recordingDeviceId == 0 ) {
   	rsError("Can't open this device");
   }
   audio->deviceId = recordingDeviceId;
   size_t size_1s = config->audioSamplerate * sizeof(uint16_t) * config->audioChannels;
   size_t size_total = size_1s * (unsigned)config->duration;

   audio->size = size_total;
   audio->data = rsMemoryCreate(size_total);
   audio->index = 0;
   audio->channels = config->audioChannels;
   audio->bitrate = config->audioBitrate;
   rsAudioPrepareEncoder(audio, config);
   return 1;
}

void rsAudioReadSamples(RSAudio *audio) {
/*   int error;
   int ret = pa_simple_read(audio->pa_api, audio->data + audio->index, audio->sizebatch,
                        &error);
   if (ret < 0) {
      rsError("PulseAudio: pa_simple_read() failed: %s", pa_strerror(error));
      return;
   }
   audio->index += audio->sizebatch;
   if (audio->index >= audio->size) {
      audio->index -= audio->size;
   }
*/}

void rsAudioEncoderCreate(RSAudioEncoder* audioenc, const RSAudio *audio, size_t rewindframes) {
	*audioenc = audio->audioenc;
	audioenc->data = rsMemoryCreate(audioenc->size);
	audioenc->frame = rsMemoryCreate(audioenc->frame_size); 
	memcpy(audioenc->data, audio->data, audio->size);
	size_t rewind_bytes = rewindframes * audio->sizebatch;
	if (rewind_bytes <= audio->index) {
		audioenc->index = audio->index - rewind_bytes;
		return;
	}
	audioenc->index = audio->size - (rewind_bytes - audio->index);
}

void rsAudioDestroy(RSAudio *audio) {
   if (audio->deviceId) {
	   SDL_CloseAudioDevice(audio->deviceId);
   }
   if (audio->data) {
      free(audio->data);
   }
}

void rsAudioEncoderDestroy(RSAudioEncoder* audioenc) {
	if (audioenc->data) {
		rsMemoryDestroy(audioenc->data);
	}
	if (audioenc->frame) {
		rsMemoryDestroy(audioenc->frame);
	}
}

void rsAudioEncodeFrame(RSAudioEncoder *audioenc, uint8_t *out, int *num_of_bytes, int *num_of_samples) {
   AACENC_BufDesc ibuf = {0};
   AACENC_BufDesc obuf = {0};
   AACENC_InArgs iargs = {0};
   AACENC_OutArgs oargs = {0};

   rsAudioEncoderPrepareFrame(audioenc);
   void *iptr = audioenc->frame;
   void *optr = out;

   int ibufsizes = audioenc->frame_size;
   int iidentifiers = IN_AUDIO_DATA;
   int ielsizes = 2;
   int obufsizes = 2048;
   int oidentifiers = OUT_BITSTREAM_DATA;
   int oelsizes = 1;

   iargs.numInSamples = audioenc->aac_info.frameLength * AUDIO_CHANNELS;
   ibuf.numBufs = 1;
   ibuf.bufs = &iptr;
   ibuf.bufferIdentifiers = &iidentifiers;
   ibuf.bufSizes = &ibufsizes;
   ibuf.bufElSizes = &ielsizes;

   obuf.numBufs = 1;
   obuf.bufs = &optr;
   obuf.bufferIdentifiers = &oidentifiers;
   obuf.bufSizes = &obufsizes;
   obuf.bufElSizes = &oelsizes;

   if (AACENC_OK !=
       aacEncEncode(audioenc->aac_enc, &ibuf, &obuf, &iargs, &oargs)) {
      rsLog("AAC: aac encode failed");
   }
   *num_of_bytes = oargs.numOutBytes;
   *num_of_samples = iargs.numInSamples;
}

static void rsAudioPrepareEncoder(RSAudio *audio, const RSConfig* config) {
   aacEncOpen(&(audio->audioenc.aac_enc), 0, 0);
   aacEncoder_SetParam(audio->audioenc.aac_enc, AACENC_TRANSMUX, 0);
   aacEncoder_SetParam(audio->audioenc.aac_enc, AACENC_AFTERBURNER, 1);
   aacEncoder_SetParam(audio->audioenc.aac_enc, AACENC_BITRATE, AUDIO_BITRATE);
   aacEncoder_SetParam(audio->audioenc.aac_enc, AACENC_SAMPLERATE, AUDIO_RATE);
   aacEncoder_SetParam(audio->audioenc.aac_enc, AACENC_CHANNELMODE, config->audioChannels);
   aacEncEncode(audio->audioenc.aac_enc, NULL, NULL, NULL, NULL);
   aacEncInfo(audio->audioenc.aac_enc, &(audio->audioenc.aac_info));

   audio->audioenc.data = 0;
   audio->audioenc.frame = 0;
   audio->audioenc.size = audio->size;
   audio->audioenc.frame_size = audio->audioenc.aac_info.frameLength * audio->channels * sizeof(uint16_t);
   audio->audioenc.index = 0;
}

static void rsAudioEncoderPrepareFrame(RSAudioEncoder *audioenc) {
   int32_t first_copy_size = audioenc->frame_size;
   int32_t end_of_data = audioenc->index + audioenc->frame_size;
   int diff = audioenc->size - end_of_data;
   if (diff >= 0) {
      memcpy(audioenc->frame, audioenc->data + audioenc->index, first_copy_size);
      audioenc->index += first_copy_size;
      return;
   }
   first_copy_size += diff;
   memcpy(audioenc->frame, audioenc->data + audioenc->index, first_copy_size);
   memcpy(audioenc->frame + first_copy_size, audioenc->data, -diff);
   audioenc->index = -diff;
}


