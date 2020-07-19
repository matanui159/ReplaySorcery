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

#include "compress.h"
#include "util/log.h"

static void compressOutputMessage(struct jpeg_common_struct *jpeg) {
   char error[JMSG_LENGTH_MAX];
   jpeg->err->format_message(jpeg, error);
   rsLog("JPEG warning: %s", error);
}

static void compressErrorExit(struct jpeg_common_struct *jpeg) {
   char error[JMSG_LENGTH_MAX];
   jpeg->err->format_message(jpeg, error);
   rsError("JPEG error: %s", error);
}

static void compressErrorCreate(struct jpeg_error_mgr *error) {
   jpeg_std_error(error);
   error->output_message = compressOutputMessage;
   error->error_exit = compressErrorExit;
}

static boolean compressDestinationEmpty(struct jpeg_compress_struct *jpeg) {
   RSCompressDestination *dest = (RSCompressDestination *)jpeg->dest;
   dest->jpeg.next_output_byte =
       rsBufferAutoGrow(dest->buffer, &dest->jpeg.free_in_buffer);
   return true;
}

static void compressDestinationInit(struct jpeg_compress_struct *jpeg) {
   compressDestinationEmpty(jpeg);
}

static void compressDestinationTerm(struct jpeg_compress_struct *jpeg) {
   RSCompressDestination *dest = (RSCompressDestination *)jpeg->dest;
   rsBufferShrink(dest->buffer, dest->jpeg.free_in_buffer);
}

static void decompressSourceInit(struct jpeg_decompress_struct *jpeg) {
   RSDecompressSource *source = (RSDecompressSource *)jpeg->src;
   source->filled = false;
}

static void decompressSourceTerm(struct jpeg_decompress_struct *jpeg) {
   (void)jpeg;
   return;
}

static boolean decompressSourceFill(struct jpeg_decompress_struct *jpeg) {
   RSDecompressSource *source = (RSDecompressSource *)jpeg->src;
   if (source->filled) {
      return true;
   }
   source->jpeg.next_input_byte = (uint8_t *)source->buffer->data;
   source->jpeg.bytes_in_buffer = source->buffer->size;
   source->filled = true;
   return true;
}

static void decompressSourceSkip(struct jpeg_decompress_struct *jpeg, long size) {
   jpeg->src->next_input_byte += size;
   jpeg->src->bytes_in_buffer -= (size_t)size;
}

void rsCompressCreate(RSCompress *compress, const RSConfig *config) {
   compressErrorCreate(&compress->error);
   compress->jpeg.err = &compress->error;
   jpeg_create_compress(&compress->jpeg);

   compress->jpeg.dest = &compress->dest.jpeg;
   compress->dest.jpeg.init_destination = compressDestinationInit;
   compress->dest.jpeg.term_destination = compressDestinationTerm;
   compress->dest.jpeg.empty_output_buffer = compressDestinationEmpty;

   compress->jpeg.input_components = 4;
   compress->jpeg.in_color_space = JCS_EXT_BGRX;
   jpeg_set_defaults(&compress->jpeg);
   jpeg_set_quality(&compress->jpeg, config->compressQuality, false);
   compress->jpeg.dct_method = JDCT_IFAST;
}

void rsCompressDestroy(RSCompress *compress) {
   jpeg_destroy_compress(&compress->jpeg);
}

void rsCompress(RSCompress *compress, RSBuffer *buffer, const RSFrame *frame) {
   compress->dest.buffer = buffer;
   compress->jpeg.image_width = (unsigned)frame->width;
   compress->jpeg.image_height = (unsigned)frame->height;
   const uint8_t *scanlines[frame->height];
   scanlines[0] = frame->data;
   for (size_t i = 1; i < frame->height; ++i) {
      scanlines[i] = scanlines[i - 1] + frame->strideY;
   }

   jpeg_start_compress(&compress->jpeg, true);
   jpeg_write_scanlines(&compress->jpeg, (uint8_t **)scanlines, (unsigned)frame->height);
   jpeg_finish_compress(&compress->jpeg);
}

void rsDecompressCreate(RSDecompress *decompress) {
   compressErrorCreate(&decompress->error);
   decompress->jpeg.err = &decompress->error;
   jpeg_create_decompress(&decompress->jpeg);

   decompress->jpeg.src = &decompress->source.jpeg;
   decompress->source.jpeg.init_source = decompressSourceInit;
   decompress->source.jpeg.term_source = decompressSourceTerm;
   decompress->source.jpeg.fill_input_buffer = decompressSourceFill;
   decompress->source.jpeg.skip_input_data = decompressSourceSkip;
   decompress->source.jpeg.resync_to_restart = jpeg_resync_to_restart;
}

void rsDecompressDestroy(RSDecompress *decompress) {
   jpeg_destroy_decompress(&decompress->jpeg);
}

void rsDecompress(RSDecompress *decompress, RSFrame *frame, const RSBuffer *buffer) {
   if (buffer != NULL) {
      decompress->source.buffer = buffer;
      if (decompress->source.filled) {
         decompress->source.filled = false;
         decompressSourceFill(&decompress->jpeg);
      }
   }
   uint8_t *scanlines[frame->height];
   scanlines[0] = frame->data;
   for (size_t i = 1; i < frame->height; ++i) {
      scanlines[i] = scanlines[i - 1] + frame->strideY;
   }

   jpeg_read_header(&decompress->jpeg, true);
   decompress->jpeg.out_color_space = JCS_YCbCr;
   jpeg_start_decompress(&decompress->jpeg);
   while (decompress->jpeg.output_scanline < frame->height) {
      jpeg_read_scanlines(&decompress->jpeg, scanlines + decompress->jpeg.output_scanline,
                          (unsigned)frame->height);
   }
   jpeg_finish_decompress(&decompress->jpeg);
}
