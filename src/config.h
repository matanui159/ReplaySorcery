#ifndef RS_CONFIG_H
#define RS_CONFIG_H
#include <libavutil/avutil.h>
#include <stdbool.h>

typedef enum RSConfigQuality {
   RS_QUALITY_LOW,
   RS_QUALITY_MEDIUM,
   RS_QUALITY_HIGH
} RSConfigQuality;

/**
 * The global configuration for the program.
 */
typedef struct RSConfig {
   /**
    * The name of the display to read from.
    */
   const char* inputDisplay;

   /**
    * The width, height and offset x, y of the display to grab frames from.
    */
   int inputWidth;
   int inputHeight;
   int inputX;
   int inputY;

   /**
    * The framerate of the input stream.
    */
   int inputFramerate;

   /**
    * The width and height to downscale to. Use this setting for high resolution displays
    * if you want faster encoding performance.
    */
   int recordWidth;
   int recordHeight;

   /**
    * The quality of the encoded video. This changes meaning depending on what encoder
    * is being used.
    *
    * Currently not implemented.
    */
   RSConfigQuality recordQuality;

   /**
    * The maximum record duration. Sets the size of the circle buffer.
    */
   int recordDuration;

   /**
    * Disable the X11 input. Since X11 is the only supported input right now, this option
    * is not recommended.
    */
   bool disableX11Input;

   /**
    * Disable different encoders backends. The order goes nVidia (NVENC), VAAPI, software.
    * For example, if you had an nVidia GPU but wanted to use VAAPI, you can set
    * `disableNvidiaEncoder`.
    */
   bool disableNvidiaEncoder;
   bool disableVaapiEncoder;
   bool disableSoftwareEncoder;

   /**
    * The name of the output file to write to.
    */
   const char* outputFile;

   /**
    * Sets the FFmpeg log level.
    */
   int logLevel;

   /**
    * Enables the debug "user". This creates a simple command-line interface where you
    * just press enter to save the video. Makes it easy for development.
    *
    * Since currently this is the only user implementation, this option does nothing.
    */
   bool enableDebug;
} RSConfig;

/**
 * The instance of the global configuration, public to all for all the bad program
 * practices to take place.
 */
extern RSConfig rsConfig;

#endif
