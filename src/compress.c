#include "compress.h"

static void compressOutputMessage(struct jpeg_common_struct *jpeg) {
   char error[JMSG_LENGTH_MAX];
   jpeg->err->format_message(jpeg, error);
   rsLog("JPEG warning: %s", error);
}

static void compressErrorExit(struct jpeg_common_struct *jpeg) {
   char error[JMSG_LENGTH_MAX];
   jpeg->err->format_message(jpeg, error);
   rsError(0, "JPEG error: %s", error);
}

static void compressDestinationGrow(RSCompressDestination *dest) {
   dest->jpeg.next_output_byte = rsBufferGetSpace(dest->buffer, &dest->size);
   dest->jpeg.free_in_buffer = dest->size;
}

static void compressDestinationInit(struct jpeg_compress_struct *jpeg) {
   RSCompressDestination *dest = (RSCompressDestination *)jpeg->dest;
   rsBufferClear(dest->buffer);
   compressDestinationGrow(dest);
}

static void compressDestinationTerm(struct jpeg_compress_struct *jpeg) {
   RSCompressDestination *dest = (RSCompressDestination *)jpeg->dest;
   rsBufferAppend(dest->buffer, dest->size - dest->jpeg.free_in_buffer);
   rsBufferOptimize(dest->buffer);
}

static boolean compressDestinationEmpty(struct jpeg_compress_struct *jpeg) {
   RSCompressDestination *dest = (RSCompressDestination *)jpeg->dest;
   rsBufferAppend(dest->buffer, dest->size);
   compressDestinationGrow(dest);
   return true;
}

void rsCompressCreate(RSCompress *compress, const RSConfig *config) {
   compress->jpeg.err = jpeg_std_error(&compress->error);
   compress->error.output_message = compressOutputMessage;
   compress->error.error_exit = compressErrorExit;
   jpeg_create_compress(&compress->jpeg);

   compress->jpeg.dest = (struct jpeg_destination_mgr *)&compress->dest;
   compress->dest.jpeg.init_destination = compressDestinationInit;
   compress->dest.jpeg.term_destination = compressDestinationTerm;
   compress->dest.jpeg.empty_output_buffer = compressDestinationEmpty;

   compress->jpeg.input_components = 4;
   compress->jpeg.in_color_space = JCS_EXT_BGRX;
   jpeg_set_defaults(&compress->jpeg);
   jpeg_set_quality(&compress->jpeg, config->compressQuality, true);
}

void rsCompressDestroy(RSCompress *compress) {
   jpeg_destroy_compress(&compress->jpeg);
}

void rsCompress(RSCompress *compress, RSBuffer *buffer, RSSystemFrame *frame) {
   compress->dest.buffer = buffer;
   compress->jpeg.image_width = (unsigned)frame->width;
   compress->jpeg.image_height = (unsigned)frame->height;
   const uint8_t *scanlines[frame->height];
   scanlines[0] = frame->data;
   for (size_t i = 1; i < frame->height; ++i) {
      scanlines[i] = scanlines[i - 1] + frame->stride;
   }

   jpeg_start_compress(&compress->jpeg, true);
   jpeg_write_scanlines(&compress->jpeg, (uint8_t **)scanlines, (unsigned)frame->height);
   jpeg_finish_compress(&compress->jpeg);
}
