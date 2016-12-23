/*
 * IChannelListener.h
 *
 *  Created on: 2016年12月21日
 *      Author: hamersun
 */

#ifndef ICHANNELLISTENER_H_
#define ICHANNELLISTENER_H_

#include <sys/types.h>

struct PeerInfo {
    uid_t uid;
    int fd;
    PeerInfo() : uid(-1), fd(-1) {}
};

class IChannelListener {
public:
    virtual ~IChannelListener() = default;

    virtual void OnConnected(struct PeerInfo peer) = 0;
    virtual void OnDisconnected(struct PeerInfo peer) = 0;
    virtual void OnStop() = 0;
    virtual void OnDataReceived(struct PeerInfo peer, char *data, const int32_t data_size) = 0;
};


#endif /* ICHANNELLISTENER_H_ */
