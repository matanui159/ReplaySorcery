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

#include "socket.h"
#include "rsbuild.h"
#include "util.h"
#include <stdio.h>
#ifdef RS_BUILD_UNIX_SOCKET_FOUND
#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#endif

#ifdef RS_BUILD_UNIX_SOCKET_FOUND
typedef int (*SocketAddressCallback)(int fd, const struct sockaddr *addr,
                                     socklen_t addrLength);

static int socketHandleAddress(RSSocket *sock, const char *path,
                               SocketAddressCallback callback) {
   struct sockaddr_un addr = {.sun_family = AF_UNIX};
   strncpy(addr.sun_path, path, 108);
   addr.sun_path[107] = '\0';
   if (callback(sock->fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
      return AVERROR(errno);
   }
   return 0;
}
#endif

int rsSocketCreate(RSSocket *sock) {
#ifdef RS_BUILD_UNIX_SOCKET_FOUND
   sock->fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
   if (sock->fd == -1) {
      int ret = AVERROR(errno);
      av_log(NULL, AV_LOG_ERROR, "Failed to create socket: %s\n", av_err2str(ret));
      return ret;
   }
   return 0;

#else
   (void)sock;
   av_log(NULL, AV_LOG_ERROR, "Unix socket was not found during compilation\n");
   return AVERROR(ENOSYS);
#endif
}

void rsSocketDestroy(RSSocket *sock) {
#ifdef RS_BUILD_UNIX_SOCKET_FOUND
   if (sock->bindPath != NULL) {
      remove(sock->bindPath);
      av_freep(&sock->bindPath);
   }
   if (sock->fd > 0) {
      close(sock->fd);
      sock->fd = -1;
   }

#else
   (void)sock;
#endif
}

int rsSocketBind(RSSocket *sock, const char *path) {
#ifdef RS_BUILD_UNIX_SOCKET_FOUND
   int ret;
   sock->bindPath = av_strdup(path);
   if (sock->bindPath == NULL) {
      return AVERROR(ENOMEM);
   }
   if ((ret = rsDirectoryCreate(path)) < 0) {
      av_log(NULL, AV_LOG_ERROR, "Failed to create socket directory: %s\n",
             av_err2str(ret));
      return ret;
   }
   if ((ret = socketHandleAddress(sock, path, bind)) < 0) {
      av_log(NULL, AV_LOG_ERROR, "Failed to bind socket: %s\n", av_err2str(ret));
      return ret;
   }
   if (listen(sock->fd, 1) == -1) {
      ret = AVERROR(errno);
      av_log(NULL, AV_LOG_ERROR, "Failed to listen for connections: %s\n",
             av_err2str(ret));
      return ret;
   }
   return 0;

#else
   (void)sock;
   (void)path;
   return AVERROR(ENOSYS);
#endif
}

int rsSocketConnect(RSSocket *sock, const char *path) {
#ifdef RS_BUILD_UNIX_SOCKET_FOUND
   int ret;
   if ((ret = socketHandleAddress(sock, path, connect)) < 0) {
      av_log(NULL, AV_LOG_ERROR, "Failed to connect socket: %s\n", av_err2str(ret));
      return ret;
   }
   return 0;

#else
   (void)sock;
   (void)path;
   return AVERROR(ENOSYS);
#endif
}

int rsSocketAccept(RSSocket *sock, RSSocket *conn, int timeout) {
#ifdef RS_BUILD_UNIX_SOCKET_FOUND
   int ret;
   if ((ret = poll(&(struct pollfd){.fd = sock->fd, .events = POLLIN}, 1, timeout)) ==
       -1) {
      if (errno == EINTR) {
         return AVERROR(EAGAIN);
      }
      ret = AVERROR(errno);
      av_log(NULL, AV_LOG_ERROR, "Failed to poll socket: %s\n", av_err2str(ret));
      return ret;
   }
   if (ret == 0) {
      return AVERROR(EAGAIN);
   }

   conn->fd = accept(sock->fd, NULL, NULL);
   if (conn->fd == -1) {
      ret = AVERROR(errno);
      av_log(NULL, AV_LOG_ERROR, "Failed to accept connection: %s\n", av_err2str(ret));
      return ret;
   }
   return 0;

#else
   (void)sock;
   (void)conn;
   (void)timeout;
   return AVERROR(ENOSYS);
#endif
}

int rsSocketSend(RSSocket *sock, size_t size, const void *buffer, size_t fileCount,
                 const int *files) {
#ifdef RS_BUILD_UNIX_SOCKET_FOUND
   if (fileCount > RS_SOCKET_MAX_FILES) {
      return AVERROR(EINVAL);
   }

   struct msghdr msg = {0};
   struct iovec *iov = &(struct iovec){.iov_base = (void *)buffer, .iov_len = size};
   if (size > 0) {
      msg.msg_iovlen = 1;
      msg.msg_iov = iov;
   }
   if (fileCount > 0) {
      size_t filesSize = sizeof(int) * fileCount;
      msg.msg_control = sock->buffer;
      msg.msg_controllen = CMSG_LEN(filesSize);
      struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
      cmsg->cmsg_len = msg.msg_controllen;
      cmsg->cmsg_level = SOL_SOCKET;
      cmsg->cmsg_type = SCM_RIGHTS;
      memcpy(CMSG_DATA(cmsg), files, 4);
   }
   if (sendmsg(sock->fd, &msg, 0) == -1) {
      int ret = AVERROR(errno);
      av_log(NULL, AV_LOG_ERROR, "Failed to send message: %s\n", av_err2str(ret));
      return ret;
   }
   return 0;

#else
   (void)sock;
   (void)size;
   (void)buffer;
   (void)fileCount;
   (void)files;
   return AVERROR(ENOSYS);
#endif
}

int rsSocketReceive(RSSocket *sock, size_t size, void *buffer, size_t fileCount,
                    int *files) {
#ifdef RS_BUILD_UNIX_SOCKET_FOUND
   if (fileCount > RS_SOCKET_MAX_FILES) {
      return AVERROR(EINVAL);
   }

   struct msghdr msg = {0};
   struct iovec *iov = &(struct iovec){.iov_base = buffer, .iov_len = size};
   size_t filesSize = sizeof(int) * fileCount;
   struct cmsghdr *cmsg = NULL;
   if (size > 0) {
      msg.msg_iovlen = 1;
      msg.msg_iov = iov;
   }
   if (fileCount > 0) {
      msg.msg_control = sock->buffer;
      msg.msg_controllen = CMSG_LEN(filesSize);
      cmsg = CMSG_FIRSTHDR(&msg);
      cmsg->cmsg_len = msg.msg_controllen;
      cmsg->cmsg_level = SOL_SOCKET;
      cmsg->cmsg_type = SCM_RIGHTS;
   }
   if (recvmsg(sock->fd, &msg, 0) == -1) {
      int ret = AVERROR(errno);
      av_log(NULL, AV_LOG_ERROR, "Failed to receive message: %s\n", av_err2str(ret));
      return ret;
   }
   if (cmsg != NULL) {
      memcpy(files, CMSG_DATA(cmsg), filesSize);
   }
   return 0;

#else
   (void)sock;
   (void)size;
   (void)buffer;
   (void)fileCount;
   (void)files;
   return AVERROR(ENOSYS);
#endif
}
