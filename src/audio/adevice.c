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

#include "adevice.h"
#include "../config.h"

int rsAudioDeviceCreate(RSDevice *device) {
   int ret;
   (void)device;
   switch (rsConfig.audioInput) {
   case RS_CONFIG_DEVICE_NONE:
      av_log(NULL, AV_LOG_WARNING, "Audio is disabled\n");
      return AVERROR(ENOSYS);
   case RS_CONFIG_DEVICE_PULSE:
      return rsPulseDeviceCreate(device);
   }

   if ((ret = rsPulseDeviceCreate(device)) >= 0) {
      av_log(NULL, AV_LOG_INFO, "Created PulseAudio device\n");
      return ret;
   }
   av_log(NULL, AV_LOG_WARNING, "Failed to create PulseAudio device: %s\n",
          av_err2str(ret));

   return AVERROR(ENOSYS);
}
