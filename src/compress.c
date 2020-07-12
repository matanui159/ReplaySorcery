#include "compress.h"

static void outputCompressMessage(struct jpeg_common_struct *jpeg) {
   char error[JMSG_LENGTH_MAX];
   jpeg->err->format_message(jpeg, error);
   rsLog("JPEG warning: %s", error);
}

static void outputCompressError(struct jpeg_common_struct *jpeg) {
   char error[JMSG_LENGTH_MAX];
   jpeg->err->format_message(jpeg, error);
   rsError(0, "JPEG error: %s", error);
}

static void growCompressDestination(RSCompressDestination *dest) {
   dest->jpeg.next_output_byte = rsGetBufferSpace(dest->buffer, &dest->size);
   dest->jpeg.free_in_buffer = dest->size;
}

static void initCompressDestination(struct jpeg_compress_struct *jpeg) {
   RSCompressDestination *dest = (RSCompressDestination*)jpeg->dest;
   rsClearBuffer(dest->buffer);
   growCompressDestination(dest);
}

static void terminateCompressDestination(struct jpeg_compress_struct *jpeg) {
   RSCompressDestination *dest = (RSCompressDestination*)jpeg->dest;
   rsAppendBuffer(dest->buffer, dest->size - dest->jpeg.free_in_buffer);
}

static boolean emptyCompressDestination(struct jpeg_compress_struct *jpeg) {
   RSCompressDestination *dest = (RSCompressDestination*)jpeg->dest;
   rsAppendBuffer(dest->buffer, dest->size);
   growCompressDestination(dest);
   return true;
}

void rsCreateCompress(RSCompress *compress, const RSConfig *config) {
   compress->jpeg.err = jpeg_std_error(&compress->error);
   compress->error.output_message = outputCompressMessage;
   compress->error.error_exit = outputCompressError;
   jpeg_create_compress(&compress->jpeg);

   compress->jpeg.dest = (struct jpeg_destination_mgr*)&compress->dest;
   compress->dest.jpeg.init_destination = initCompressDestination;
   compress->dest.jpeg.term_destination = terminateCompressDestination;
   compress->dest.jpeg.empty_output_buffer = emptyCompressDestination;

   compress->jpeg.input_components = 4;
   compress->jpeg.in_color_space = JCS_EXT_BGRX;
   jpeg_set_defaults(&compress->jpeg);
   jpeg_set_quality(&compress->jpeg, config->compressQuality, true);
}

void rsDestroyCompress(RSCompress *compress) {
   jpeg_destroy_compress(&compress->jpeg);
}

void rsCompressSystemFrame(RSCompress *compress, RSBuffer *buffer, RSSystemFrame *frame) {
   compress->dest.buffer = buffer;
   compress->jpeg.image_width = (unsigned)frame->width;
   compress->jpeg.image_height = (unsigned)frame->height;
   const uint8_t *scanlines[frame->height];
   scanlines[0] = frame->data;
   for (size_t i = 1; i < frame->height; ++i) {
      scanlines[i] = scanlines[i - 1] + frame->stride;
   }

   jpeg_start_compress(&compress->jpeg, true);
   jpeg_write_scanlines(&compress->jpeg, (uint8_t**)scanlines, (unsigned)frame->height);
   jpeg_finish_compress(&compress->jpeg);
}
