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
#include "../util/log.h"
#include "../util/memory.h"

void rsAudioEncoderCreate(RSAudioEncoder *audioenc, const RSConfig *config) {
   if (AACENC_OK != aacEncOpen(&(audioenc->aac_enc), 0, 0)) {
      rsError("aacEncOpen failed");
   }
   aacEncoder_SetParam(audioenc->aac_enc, AACENC_TRANSMUX, 0);
   aacEncoder_SetParam(audioenc->aac_enc, AACENC_AFTERBURNER, 1);
   aacEncoder_SetParam(audioenc->aac_enc, AACENC_BITRATE, (UINT)config->audioBitrate);
   aacEncoder_SetParam(audioenc->aac_enc, AACENC_SAMPLERATE,
                       (UINT)config->audioSamplerate);
   aacEncoder_SetParam(audioenc->aac_enc, AACENC_CHANNELMODE,
                       (UINT)config->audioChannels);
   if (AACENC_OK != aacEncEncode(audioenc->aac_enc, NULL, NULL, NULL, NULL)) {
      rsError("aacEncEncode failed. Probably bad bitrate. Tried %d",
              config->audioBitrate);
   }
   aacEncInfo(audioenc->aac_enc, &(audioenc->aac_info));
   audioenc->samplesPerFrame =
       (int)audioenc->aac_info.frameLength * config->audioChannels;
   audioenc->frameSize = audioenc->samplesPerFrame * (int)sizeof(uint16_t);
}

void rsAudioEncoderEncode(RSAudioEncoder *audioenc, uint8_t *rawSamples, uint8_t *out,
                          int *numBytes, int *numSamples) {
   AACENC_BufDesc ibuf = {0};
   AACENC_BufDesc obuf = {0};
   AACENC_InArgs iargs = {0};
   AACENC_OutArgs oargs = {0};

   void *iptr = rawSamples;
   void *optr = out;

   int ibufsizes = audioenc->frameSize;
   int iidentifiers = IN_AUDIO_DATA;
   int ielsizes = 2;
   int obufsizes = AAC_OUTPUT_BUFFER_SIZE;
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

   if (aacEncEncode(audioenc->aac_enc, &ibuf, &obuf, &iargs, &oargs) != AACENC_OK) {
      rsLog("AAC: aac encode failed");
   }
   *numBytes = oargs.numOutBytes;
   *numSamples = iargs.numInSamples;
}
