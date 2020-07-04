/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "record.h"
#include "error.h"
#include "input/input.h"
#include "input/video.h"
#include "encoder/video.h"
#include <stdbool.h>
#include <pthread.h>
#include <libavutil/avutil.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>

static struct {
   SRInput input;
   SREncoder encoder;
   pthread_t thread;
   bool exit;
} priv;

static void* recordThread(void* arg) {
   (void)arg;
   AVFrame* frame = av_frame_alloc();

   while (!priv.exit) {
      srInputRead(&priv.input, frame);
      srEncode(&priv.encoder, frame);
   }
   return NULL;
}

void srRecordInit(void) {
   srInputInit();
   srVideoInputCreate(&priv.input);
   srVideoEncoderCreate(&priv.encoder, &priv.input);
   srCheckPosix(pthread_create(&priv.thread, NULL, recordThread, NULL));
}

void srRecordExit(void) {
   priv.exit = true;
   pthread_join(priv.thread, NULL);
   srEncoderDestroy(&priv.encoder);
   srInputDestroy(&priv.input);
}

const SREncoder* srRecordVideo(void) {
   return &priv.encoder;
}
