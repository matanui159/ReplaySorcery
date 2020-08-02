/*
 * Copyright (C) 2020 Patryk Seregiet
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

#include "audio_encoder.h"
#include "audio.h"
#include "util/log.h"
#include "util/memory.h"

void rsAudioEncoderCreate(RSAudioEncoder* audioenc, RSAudio *audio, int rewindFrames) {
   pthread_spin_lock(&audio->sampleGetLock);
   *audioenc = audio->audioenc;
   rsCircleStaticDuplicate(&audioenc->data, &audio->data);
   int rewindBytes = rewindFrames * audio->sizeBatch;
   rsCircleStaticMoveBackIndex(&audioenc->data, rewindBytes);
   audioenc->frame = rsMemoryCreate((size_t)audioenc->frameSize); 
   pthread_spin_unlock(&audio->sampleGetLock);
}

void rsAudioEncoderDestroy(RSAudioEncoder* audioenc) {
   if (audioenc->frame) {
      rsMemoryDestroy(audioenc->frame);
   }
   rsCircleStaticDestroy(&audioenc->data);
}

void rsAudioEncodeFrame(RSAudioEncoder *audioenc, uint8_t *out, int *num_of_bytes, int *num_of_samples) {
   AACENC_BufDesc ibuf = {0};
   AACENC_BufDesc obuf = {0};
   AACENC_InArgs iargs = {0};
   AACENC_OutArgs oargs = {0};

   //prepareFrame(audioenc);
   rsCircleStaticGet(&audioenc->data, audioenc->frame, audioenc->frameSize);
   void *iptr = audioenc->frame;
   void *optr = out;

   int ibufsizes = audioenc->frameSize;
   int iidentifiers = IN_AUDIO_DATA;
   int ielsizes = 2;
   int obufsizes = 2048;
   int oidentifiers = OUT_BITSTREAM_DATA;
   int oelsizes = 1;

   iargs.numInSamples = audioenc->samplesPerFrame;
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

