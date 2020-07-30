#include "audio.h"

/*
int main(int argc, char** argv[])
{
	RSAudio audio;
	RSConfig config;
	config.framerate = 30;
	config.duration = 10;
	rsAudioCreate(&audio, &config);
	rsAudioEncode(&audio);
}
*/

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
/*
	size_t size_1s = 88200;
	size_t size_total = 2646000;
*/
	printf("one second: %d, total: %d\n", size_1s, size_total);
	audio->one_second_size = size_1s;
	audio->size = size_total;
	audio->data = malloc(size_total + 1024);
	if (!audio->data) {
		printf("audio malloc failed\n");
		exit(1);
	}
	audio->index = 0;
	rsAudioPrepareEncoder(audio);
	printf("rsAudioCreate\n");
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
		printf("legit\n");
		return;
	}
	first_copy_size += diff;
	memcpy(dst, audio->data + audio->index, first_copy_size);
	memcpy(dst + first_copy_size, audio->data, -diff);
	audio->index = -diff;
}

void rsAudioRewindBuffer(RSAudio* audio, int seconds) {
	size_t total_size = audio->one_second_size * seconds;
	int new_index = audio->index - total_size;
	if (new_index < 0) {
		new_index = audio->size + new_index;
	}
	audio->index = 0;
	printf("index is now at %d\n", audio->index);
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
	printf("grabbed 1 sec of audio\n");
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
