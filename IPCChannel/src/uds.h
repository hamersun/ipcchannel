/*
 * uds.h
 *
 *  Created on: 2016年12月22日
 *      Author: hamersun
 */

#ifndef UDS_H_
#define UDS_H_
#include <sys/types.h>
#include <sys/select.h>

// server
int serv_listen(const char *server_sock_file);
int serv_accept(int listenFd, uid_t *uidPtr);
void serv_teardown(int listenFd);

// client
int connect_server(const char *server_sock_file);

// common
int recv_data(int fd, char *data, const int max_size);
int send_data(int fd, const char *data, const int data_size);
void close_connection(int fd);
uid_t get_peer_uid(int fd);

#endif /* UDS_H_ */
