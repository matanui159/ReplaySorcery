/*
 * Copyright (C) 2021  Joshua Minter
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

#ifndef RS_SOCKET_H
#define RS_SOCKET_H
#include "rsbuild.h"
#include <libavutil/avutil.h>
#ifdef RS_BUILD_UNIX_SOCKET_FOUND
#include <sys/socket.h>
#endif

#define RS_SOCKET_MAX_FILES 4
#ifdef RS_BUILD_UNIX_SOCKET_FOUND
#define RS_SOCKET_BUFFER_SIZE CMSG_SPACE(sizeof(int) * RS_SOCKET_MAX_FILES)
#else
#define RS_SOCKET_BUFFER_SIZE 1
#endif

typedef struct RSSocket {
   int fd;
   char *bindPath;
   char buffer[RS_SOCKET_BUFFER_SIZE];
} RSSocket;

int rsSocketCreate(RSSocket *sock);
void rsSocketDestroy(RSSocket *sock);
int rsSocketBind(RSSocket *sock, const char *path);
int rsSocketConnect(RSSocket *sock, const char *path);
int rsSocketAccept(RSSocket *sock, RSSocket *conn, int timeout);
int rsSocketSend(RSSocket *sock, size_t size, const void *buffer, size_t fileCount,
                 const int *files);
int rsSocketReceive(RSSocket *sock, size_t size, void *buffer, size_t fileCount,
                    int *files);

#endif
