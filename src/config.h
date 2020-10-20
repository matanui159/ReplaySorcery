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

#ifndef RS_CONFIG_H
#define RS_CONFIG_H
#include <libavutil/avutil.h>

#define RS_CONFIG_AUTO -1

#define RS_CONFIG_VIDEO_X11 0
#define RS_CONFIG_VIDEO_X264 0

#define RS_CONFIG_PRESET_FAST 0
#define RS_CONFIG_PRESET_MEDIUM 1
#define RS_CONFIG_PRESET_SLOW 2

#define RS_CONFIG_CONTROL_DEBUG 0
#define RS_CONFIG_CONTROL_X11 1

typedef struct RSConfig {
   const AVClass *avClass;
   int logLevel;
   int traceLevel;
   int recordSeconds;
   int videoX;
   int videoY;
   int videoWidth;
   int videoHeight;
   int videoFramerate;
   int videoInput;
   int videoEncoder;
   int videoProfile;
   int videoPreset;
   int videoQuality;
   int videoThreads;
   int videoScaler;
   int controller;
   char *outputPath;
} RSConfig;

extern RSConfig rsConfig;

int rsConfigInit(void);
void rsConfigExit(void);

#endif
