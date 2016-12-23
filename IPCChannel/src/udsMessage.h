/*
 * udsMessage.h
 *
 *  Created on: 2016年12月21日
 *      Author: hamersun
 */

#ifndef UDSMESSAGE_H_
#define UDSMESSAGE_H_

enum _ipc_type {
    IPC_REQ = 1,  // request
    IPC_RET = 2,  // return of request
    IPC_EVT = 3, // event notification
} ipc_type;

struct ipc_msg {
    unsigned int cmd;
    ipc_type type;
    int len;
    char data[0];
};


#endif /* UDSMESSAGE_H_ */
