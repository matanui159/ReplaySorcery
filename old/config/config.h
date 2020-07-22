/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef RS_CONFIG_H
#define RS_CONFIG_H
#include <libavutil/avutil.h>
#include <stdbool.h>

/**
 * Used for `recordQuality` to define the encoding quality. We have this instead of
 * specific settings to make things easier and also because each encoder has a different
 * set of settings.
 */
typedef enum RSConfigQuality {
   /**
    * Very fast with low resource usage.
    */
   RS_QUALITY_LOW,

   /**
    * Somewhere inbetween low and high.
    */
   RS_QUALITY_MEDIUM,

   /**
    * High resource usage. May introduce lag in the output video.
    */
   RS_QUALITY_HIGH
} RSConfigQuality;

/**
 * The global configuration for the program. The configuration is read from the following
 * files:
 * - $XDG_CONFIG_HOME/replay-sorcery.conf ($XDG_CONFIG_HOME defaults to ~/.config)
 * - $XDG_CONFIG_DIRS/replay-sorcery.conf ($XDG_CONFIG_DIRS defaults to /etc/xdg)
 * - <current working directory>/replay-sorcery.conf
 *
 * The file should consist of lines in the following format:
 * <name> = <value> # <optional comment>
 *
 * Thus an example configuration file could be:
 * inputDisplay = :0.1
 * inputWidth = 1920
 * inputHeight = 1080
 * recordQuality = medium
 * disableNvidiaEncoder = true # nVidia sux
 */
typedef struct RSConfig {
   /**
    * The avutil class used internall to easily set options. Is not modifyable by
    * the configuration file and should be ignored by users.
    */
   const AVClass *avclass;

   /**
    * The name of the display to read from. Default is `:0.0`.
    */
   const char *inputDisplay;

   /**
    * The width, height and offset x, y of the display to grab frames from. Default is the
    * top left-most 1080p portion of the display (1920, 1080, 0, 0).
    */
   int inputWidth;
   int inputHeight;
   int inputX;
   int inputY;

   /**
    * The framerate of the input stream. Default is 30.
    */
   int inputFramerate;

   /**
    * The width and height to downscale to. Use this setting for high resolution displays
    * if you want faster encoding performance. Default is the input width and height.
    */
   int recordWidth;
   int recordHeight;

   /**
    * The quality of the encoded video. This changes meaning depending on what encoder
    * is being used. The configuration file uses the strings "low", "medium" and "high".
    * Default is low quality.
    */
   RSConfigQuality recordQuality;

   /**
    * The maximum record duration in seconds. Sets the size of the circle buffer. Default
    * is 30 seconds.
    */
   int recordDuration;

   /**
    * Disable the X11 input. Since X11 is the only supported input right now, this option
    * is not recommended. Default is false.
    */
   int disableX11Input;

   /**
    * Disable different encoders backends. The order goes nVidia (NVENC), VAAPI, software.
    * For example, if you had an nVidia GPU but wanted to use VAAPI, you can set
    * `disableNvidiaEncoder`. Default is false for all of these.
    */
   int disableNvidiaEncoder;
   int disableVaapiEncoder;
   int disableSoftwareEncoder;

   /**
    * Disable the X11 user implementation. Since X11 is the only supported implementation
    * right now, this option is not recommended unless `enableDebug` is true. Default is
    * false.
    */
   int disableX11User;

   /**
    * The name of the output file to write to. The name is passed through strftime to
    * generate time-unique names. Default is `~/Videos/ReplaySorcery_%F_%H-%M-%S.mp4`.
    */
   const char *outputFile;

   /**
    * Sets the FFmpeg log level. The configuration file uses the strings "quiet", "panic",
    * "fatal", "error", "warning", "info", "verbose", "debug" and "trace". Default is
    * verbose logging.
    */
   int logLevel;

   /**
    * Enables the debug "user". This creates a simple command-line interface where you
    * just press enter to save the video. Makes it easy for development.
    */
   int enableDebug;
} RSConfig;

/**
 * The instance of the global configuration, public to all for all the bad program
 * practices to take place.
 */
extern RSConfig rsConfig;

/**
 * Reads the configuration files and sets the values in the above global object.
 */
void rsConfigInit(void);

#endif
