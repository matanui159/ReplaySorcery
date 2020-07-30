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

static void rsAudioPrepareEncoder(RSAudio* audio);
static void rsAudioGetSampleData(RSAudio* audio, uint8_t* dst, size_t size);

int rsAudioCreate(RSAudio* audio, const RSConfig *config) {
	int ret = 1;
	int error;
	static const pa_sample_spec ss = {
		.format = PA_SAMPLE_S16LE,
		.rate = AUDIO_RATE,
		.channels = AUDIO_CHANNELS
	};
	audio->pa_api = pa_simple_new(NULL, "Replay Sorcerer", PA_STREAM_RECORD, NULL, "record", &ss,
			NULL, NULL, &error);
	
	if (!audio->pa_api) {
		rsError("PulseAudio: pa_simple_new() failed: %s\n", pa_strerror(error));
		return 0;
	}
	size_t size_1s = pa_bytes_per_second(&ss);
	size_t size_total = size_1s * config->duration;
	audio->one_second_size = size_1s;
	audio->size = size_total;
	audio->data = malloc(size_total + 1024);
	if (!audio->data) {
		printf("audio malloc failed\n");
		exit(1);
	}
	audio->index = 0;
	rsAudioPrepareEncoder(audio);
	return 1;
}

static void rsAudioPrepareEncoder(RSAudio* audio) {
	aacEncOpen(&(audio->aac_enc), 0, 0);
	aacEncoder_SetParam(audio->aac_enc, AACENC_TRANSMUX, 0);
	aacEncoder_SetParam(audio->aac_enc, AACENC_AFTERBURNER, 1);
	aacEncoder_SetParam(audio->aac_enc, AACENC_BITRATE, AUDIO_BITRATE);
	aacEncoder_SetParam(audio->aac_enc, AACENC_SAMPLERATE, AUDIO_RATE);
	aacEncoder_SetParam(audio->aac_enc, AACENC_CHANNELMODE, MODE_1);
	aacEncEncode(audio->aac_enc, NULL, NULL, NULL, NULL);
	aacEncInfo(audio->aac_enc, &(audio->aac_info));
}

static void rsAudioGetSampleData(RSAudio* audio, uint8_t* dst, size_t size) {
	size_t first_copy_size = size;
	size_t end_of_data = audio->index + size;
	int diff = audio->size - first_copy_size;
	if (diff >= 0) {
		memcpy(dst, audio->data + audio->index, first_copy_size);
		audio->index += first_copy_size;
		return;
	}
	first_copy_size += diff;
	memcpy(dst, audio->data + audio->index, first_copy_size);
	memcpy(dst + first_copy_size, audio->data, -diff);
	audio->index = -diff;
}

void rsAudioRewindBuffer(RSAudio* audio, int frames, int framerate) {
	//TODO
	(void)frames;
	(void)framerate;
	audio->index = 0;
}

static size_t rsAudioNextSampleIndex(RSAudio* audio, size_t samples_size) {
	size_t new_index = audio->index + samples_size*audio->one_second_size;
	if (audio->index >= audio->size) {
		audio->index -= audio->size;
	}
	return new_index;
}

void rsAudioDestroy(RSAudio* audio) {
	if (audio->pa_api) {
		pa_simple_free(audio->pa_api);
	}
	if (audio->data) {
		free(audio->data);
	}
}

void rsAudioGrabSample(RSAudio* audio) {
	int ret;
	int error;
	ret = pa_simple_read(audio->pa_api, audio->data+audio->index,
			audio->one_second_size, &error);
	if (ret < 0)
	{
		rsError("PulseAudio: pa_simple_read() failed: %s\n", pa_strerror(error));
		return;
	}
	audio->index += audio->one_second_size;
	if (audio->index >= audio->size)
	{
		audio->index -= audio->size;
	}
}

int rsAudioEncode(RSAudio* audio, uint8_t *out_buffer, size_t out_buffer_size) {
	AACENC_BufDesc in_buf = {0}, out_buf = {0};
	AACENC_InArgs in_args = {0};
	AACENC_OutArgs out_args = {0};

	uint8_t src_buffer[1024*8];
	size_t in_bufsizes=audio->aac_info.frameLength * AUDIO_CHANNELS *2;
	rsAudioGetSampleData(audio, src_buffer, in_bufsizes);

	void *in_ptr = src_buffer;
	void *out_ptr = out_buffer;

	int in_identifiers = IN_AUDIO_DATA;
	int in_elsizes = 2;
	int out_bufsizes = out_buffer_size;
	int out_identifiers = OUT_BITSTREAM_DATA;
	int out_elsizes = 1;

	in_args.numInSamples = audio->aac_info.frameLength * AUDIO_CHANNELS;
	printf("%d\n", in_args.numInSamples);
	in_buf.numBufs = 1;
	in_buf.bufs = &in_ptr;
	in_buf.bufferIdentifiers = &in_identifiers;
	in_buf.bufSizes = &in_bufsizes;
	in_buf.bufElSizes = &in_elsizes;

	out_buf.numBufs = 1;
	out_buf.bufs = &out_ptr;
	out_buf.bufferIdentifiers = &out_identifiers;
	out_buf.bufSizes = &out_bufsizes;
	out_buf.bufElSizes = &out_elsizes;

	if (AACENC_OK != aacEncEncode(audio->aac_enc, &in_buf, &out_buf, &in_args, &out_args)) {
		rsError("AAC: aac encode failed\n");
		exit(1);
	}
	return out_args.numOutBytes;
}