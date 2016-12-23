/*
 * udsServer.cpp
 *
 *  Created on: 2016年12月21日
 *      Author: hamersun
 */

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>

#define QLEN 5

int serv_listen(const char *server_sock_file)
{
    int fd;
    int len;
    struct sockaddr_un addr;
    int ret;
    if (nullptr == server_sock_file) {
        perror("invalid server sock file: nullptr");
        return -1;
    }
    // create a UNIX domain stream socket
    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror("create socket error");
        return -2;
    }
    unlink(server_sock_file); // in case it already exists

    // fill in socket address structure
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, server_sock_file, sizeof(addr.sun_path)-1);
    len = offsetof(struct sockaddr_un, sun_path) + strlen(server_sock_file);

    // bind the name to 'server_sock_file' the description
    if (bind(fd, (struct sockaddr *)&addr, len) == -1) {
        fprintf(stderr, "bind error: %d(%s)\n", errno, strerror(errno));
        ret = -3;
        goto FAIL;
    }

    if (listen(fd, QLEN) < 0) {
        fprintf(stderr, "listen failed: %d(%s)\n", errno, strerror(errno));
        ret = -4;
        goto FAIL;
    }

    return fd;

 FAIL:
     close(fd);
     return ret;
}

int serv_accept(int listenFd, uid_t *uidPtr)
{
    int clientFd;
    unsigned int len;
    int ret;
    struct sockaddr_un addr;
    struct stat statBuf;

    len = sizeof(addr);
    if ((clientFd = accept(listenFd, (struct sockaddr *)&addr, &len)) < 0) {
        return -1; // often errno = EINTR, if signal caught
    }

    // obtain the client's uid from its calling address
    len -= offsetof(struct sockaddr_un, sun_path); /* len of pathname */
    addr.sun_path[len] = 0;

    if (stat(addr.sun_path, &statBuf) < 0) {
        ret = -2;
        goto FAIL;
    }

    if (S_ISSOCK(statBuf.st_mode) == 0) {
        ret = -3; // not a socket
        goto FAIL;
    }

    if (uidPtr != nullptr) {
        *uidPtr = statBuf.st_uid;
    }

    unlink(addr.sun_path); // we're done with pathname now
    return clientFd;

FAIL:
    close(clientFd);
    return ret;
}

void serv_teardown(int listenFd)
{
    if (listenFd >= 0) {
        struct sockaddr_un addr;
        unsigned int len = sizeof(addr);
        if (getsockname(listenFd, (struct sockaddr*)&addr, &len) == 0) {
            len -= offsetof(struct sockaddr_un, sun_path); /* len of pathname */
            addr.sun_path[len] = 0;
            unlink(addr.sun_path);
        }
        close(listenFd);
    }
}
