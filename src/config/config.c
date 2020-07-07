#include "config.h"

// clang-format off
RSConfig rsConfig = {
   .inputDisplay = ":0.0",
   .inputWidth = 1920,
   .inputHeight = 1080,
   .inputX = 0,
   .inputY = 0,
   .inputFramerate = 30,

   .recordWidth = 1920,
   .recordHeight = 1080,
   .recordQuality = RS_QUALITY_LOW,
   .recordDuration = 30,

   .disableX11Input = false,
   .disableNvidiaEncoder = false,
   .disableVaapiEncoder = false,
   .disableSoftwareEncoder = false,

   .outputFile = "recording.mp4",

   .logLevel = AV_LOG_VERBOSE,
   .enableDebug = false
};
// clang-format on
