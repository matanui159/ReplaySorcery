/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef SR_INPUT_H
#define SR_INPUT_H
#include <libavutil/avutil.h>
#include <libavutil/dict.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>

typedef struct SRInput {
   AVFormatContext* formatCtx;
   AVCodecContext* codecCtx;
   AVPacket* packet;
} SRInput;

void srInputInit(void);
int srInputCreate(SRInput* input, const char* name, const char* url, AVDictionary** options);
void srInputDestroy(SRInput* input);
void srInputRead(SRInput* input, AVFrame* frame);

#endif
