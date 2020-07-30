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
	size_t size;
	size_t index;
	size_t one_second_size;
} RSAudio;


int rsAudioCreate(RSAudio* audio, const RSConfig *config);
void rsAudioDestroy(RSAudio* audio);
void rsAudioGrabSample(RSAudio* audio);
int rsAudioEncode(RSAudio* audio, uint8_t *out_buffer, size_t out_buffer_size);
void rsAudioRewindBuffer(RSAudio* audio, int frames, int framerate);

#endif

