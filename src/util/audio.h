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

#ifndef RS_UTIL_AUDIO_H
#define RS_UTIL_AUDIO_H

#include <pulse/simple.h>
#include <pulse/error.h>
#include <fdk-aac/aacenc_lib.h>
#include "../config.h"

#define AUDIO_RATE 44100
#define AUDIO_CHANNELS 1
#define AUDIO_BITRATE 96000

typedef struct RSAudio {
	pa_simple *pa_api;
	HANDLE_AACENCODER aac_enc;
	AACENC_InfoStruct aac_info;
	
	uint8_t *data;
	uint8_t *encoder_data;
	size_t size;
	size_t index;
	size_t frame_size;
	size_t encoder_index;
} RSAudio;


int rsAudioCreate(RSAudio* audio, const RSConfig *config);
void rsAudioDestroy(RSAudio* audio);
void rsAudioGrabSample(RSAudio* audio);
int rsAudioEncode(RSAudio* audio, uint8_t *out_buffer, size_t out_buffer_size);
void rsAudioRewindBuffer(RSAudio* audio, int frames, int framerate);

#endif

