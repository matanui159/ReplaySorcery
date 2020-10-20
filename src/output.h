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

#ifndef RS_OUTPUT_H
#define RS_OUTPUT_H
#include "encoder/encoder.h"
#include "rsbuild.h"
#include "util/pktcircle.h"
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#ifdef RS_BUILD_PTHREAD_FOUND
#include <pthread.h>
#endif

typedef struct RSOutputParams {
   const RSEncoder *videoEncoder;
   const RSPktCircle *videoCircle;
} RSOutputParams;

typedef struct RSOutput {
   AVFormatContext *formatCtx;
   RSPktCircle videoCircle;
   size_t index;
   volatile int ret;
#ifdef RS_BUILD_PTHREAD_FOUND
   pthread_t thread;
#endif
} RSOutput;

#define RS_OUTPUT_INIT                                                                   \
   { NULL }

int rsOutputCreate(RSOutput *output, const RSOutputParams *params);
void rsOutputDestroy(RSOutput *output);
int rsOutputRun(RSOutput *output);

#endif
