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

#include "device/demux.h"
#include "util/log.h"
#include <libavdevice/avdevice.h>
#include <libavutil/avutil.h>

int main(int argc, char *argv[]) {
   (void)argc;
   (void)argv;
   rsLogInit();
   av_log_set_level(AV_LOG_DEBUG);
   avdevice_register_all();

   RSDevice device;
   rsDemuxDeviceCreate(&device, "x11grab", ":0", NULL);
   rsDeviceDestroy(&device);
}
