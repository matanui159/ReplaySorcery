/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "record.h"
#include "encoder/video.h"
#include "error.h"
#include "input/input.h"
#include "input/video.h"
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <pthread.h>
#include <stdbool.h>

static struct {
   RSInput input;
   RSEncoder encoder;
   pthread_t thread;
   bool exit;
} priv;

static void* recordThread(void* arg) {
   (void)arg;
   AVFrame* frame = av_frame_alloc();

   while (!priv.exit) {
      rsInputRead(&priv.input, frame);
      rsEncode(&priv.encoder, frame);
   }
   return NULL;
}

void rsRecordInit(void) {
   rsInputInit();
   rsVideoInputCreate(&priv.input);
   rsVideoEncoderCreate(&priv.encoder, &priv.input);
   rsCheckPosix(pthread_create(&priv.thread, NULL, recordThread, NULL));
}

void rsRecordExit(void) {
   priv.exit = true;
   pthread_join(priv.thread, NULL);
   rsEncoderDestroy(&priv.encoder);
   rsInputDestroy(&priv.input);
}

const RSEncoder* rsRecordVideo(void) {
   return &priv.encoder;
}
