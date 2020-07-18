#include "path.h"
#include "log.h"
#include <time.h>

#define PATH_DATED_MIN 16
#define PATH_DATED_GROW 2

static void pathPrint(RSBuffer *path, const char *fmt, ...) {
   va_list args;
   va_start(args, fmt);
   int ret = vsnprintf(NULL, 0, fmt, args);
   va_end(args);
   if (ret < 0) {
      rsError("Invalid print format: %s", fmt);
   }

   size_t size = (size_t)ret + 1;
   char *data = rsBufferGrow(path, size);
   va_start(args, fmt);
   vsnprintf(data, size, fmt, args);
   va_end(args);
   // Remove NULL terminator from size
   --path->size;
}

void rsPathClear(RSBuffer *path) {
   path->data[0] = '\0';
   path->size = 0;
}

void rsPathAppend(RSBuffer *path, const char *value) {
   if (value[0] == '/') {
      // Absolute path
      rsPathClear(path);
   }
   if (value[0] == '~' && value[1] == '/') {
      // Home-relative path
      rsPathAppend(path, getenv("HOME"));
      value += 2;
   }

   if (path->data[path->size - 1] == '/') {
      // Already has slash
      pathPrint(path, "%s", value);
   } else {
      // Need to append slash
      pathPrint(path, "/%s", value);
   }
}

void rsPathAppendDated(RSBuffer *path, const char *value) {
   time_t timeNum = time(NULL);
   struct tm *timeObj = localtime(&timeNum);
   size_t size = PATH_DATED_MIN;
   for (;;) {
      char buffer[size];
      if (strftime(buffer, size, value, timeObj) > 0) {
         rsPathAppend(path, buffer);
         return;
      }

      // Retry with larger buffer
      size *= 2;
   }
}
