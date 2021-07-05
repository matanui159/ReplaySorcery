/*
 * Copyright (C) 2020  <NAME>
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

#ifndef RS_CONTROL_CMDCTRL_H
#define RS_CONTROL_CMDCTRL_H
#include "control.h"
#include "rsbuild.h"
#ifdef RS_BUILD_UNISTD_FOUND
#include <unistd.h>
#endif
#ifdef RS_BUILD_SYS_SOCKET_FOUND
#include <sys/socket.h>
#endif
#ifdef RS_BUILD_SYS_UN_FOUND
#include <sys/un.h>
#endif

#if defined(RS_BUILD_UNISTD_FOUND) && defined(RS_BUILD_SYS_SOCKET_FOUND) && defined(RS_BUILD_SYS_UN_FOUND)
#define RS_COMMAND_SUPPORTED
#endif

#define RS_COMMAND_PATH "/tmp/replay-sorcery/control.sock"

#endif
