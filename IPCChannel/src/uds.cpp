/*
 * uds.cpp
 *
 *  Created on: 2016年12月22日
 *      Author: hamersun
 */

#include "uds.h"
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>

int recv_data(int fd, char *data, const int max_size)
{
    if (fd >= 0) {
        return recv(fd, data, max_size, 0);
    }
    return -1;
}

int send_data(int fd, const char *data, const int data_size)
{
    if (fd >=0) {
        return send(fd, data, data_size, 0);
    }
    return -1;
}

void close_connection(int fd)
{
    if (fd >= 0) {
        close(fd);
    }
}

uid_t get_peer_uid(int fd)
{
    struct ucred ucred;
    unsigned int len;
    len = sizeof(ucred);
    if (getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &ucred, &len) == -1) {
        return -1;
    }
    return ucred.uid;
}
