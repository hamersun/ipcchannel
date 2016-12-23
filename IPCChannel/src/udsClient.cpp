/*
 * udsClient.cpp
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
#include <sys/un.h>

#define CLI_PATH "/data/tmp/"  // +5 for pid = 14 chars

int _setup_client();
int _connect_server(int fd, const char *server_sock_file);

int connect_server(const char *server_sock_file)
{
    int fd;
    int ret;
    if ((ret = _setup_client()) < 0) {
        return ret;
    }
    fd = ret;
    return _connect_server(fd, server_sock_file);
}

//=================== internal functions =============================
int _setup_client()
{
    int cli_fd = -1;
    int len;
    struct sockaddr_un addr;
    int ret;

    // create a UNIX domain stream socket
    if ((cli_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "create socket failed: %s\n", strerror(errno));
        return -1;
    }

    // fill socket address structure with our address
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    sprintf(addr.sun_path, "%s%05d", CLI_PATH, getpid());
    len = offsetof(struct sockaddr_un, sun_path) + strlen(addr.sun_path);

    unlink(addr.sun_path); // in case it already exists

    if (bind(cli_fd, (struct sockaddr*)&addr, len) == -1) {
        fprintf(stderr, "bind error: %s\n", strerror(errno));
        ret = -2;
        goto FAIL_SETUP;
    }

    return cli_fd;

FAIL_SETUP:
    close(cli_fd);
    return ret;
}

int _connect_server(int fd, const char *server_sock_file)
{
    int ret;
    int len;
    struct sockaddr_un addr;

    if (server_sock_file == nullptr) {
        perror("invalid server address: nullptr");
        return -1;
    }

    // fill socket address structure with server's address
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, server_sock_file, sizeof(addr.sun_path)-1); // connect to server
    len = offsetof(struct sockaddr_un, sun_path) + strlen(server_sock_file);

    if (connect(fd, (struct sockaddr *)&addr, len) == -1) {
        fprintf(stderr, "failed to connect to server %s: %d(%s)\n", server_sock_file, errno, strerror(errno));
        ret = -2;
        goto FAIL_CONN;
    }

    return fd;

 FAIL_CONN:
     close(fd);
     return ret;
}
