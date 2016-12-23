/*
 * IPCChannel.h
 *
 *  Created on: 2016年12月21日
 *      Author: hamersun
 */

#ifndef IPCCHANNEL_H_
#define IPCCHANNEL_H_

#include <memory>
#include <vector>

enum ipc_role {
    SERVER = 1,
    CLIENT = 2,
};

#define IPC_ERROR 0x80001000
enum ipc_status {
    IPC_SUCCESS = 0,
    IPC_FAILED_GENERAL = IPC_ERROR,
    IPC_FAILED_SETUP_LISTEN = IPC_ERROR - 1,
    IPC_FAILED_ACCEPT = IPC_ERROR - 2,
    IPC_FAILED_CONNECTION = IPC_ERROR - 3,
};

class IChannelListener;
class IPCChannel : public std::enable_shared_from_this<IPCChannel> {
public:
    typedef std::shared_ptr<IPCChannel> pointer;
    virtual ~IPCChannel();

    static pointer create(ipc_role role = CLIENT);

    ipc_status setup(const char *comm_file);
    ipc_status teardown();
    void setChannelListener(std::shared_ptr<IChannelListener> listener);
    int sendData(struct PeerInfo &peer, const char *data, const int32_t data_size);
    ipc_role role();

private:
    IPCChannel(ipc_role role);

    int _setup_server();
    int _setup_client();
    int _handle_new_connection(PeerInfo &info);
    void _close();
    void _handleCommunicationAsServer();
    void _handleCommunicationAsClient();
    int _recv_data(std::vector<PeerInfo> &list, int fd, char *buf, const int max_size);

    ipc_role _mRole;
    std::string _mCommAddress;
    bool _mStop;
    int _mFd;
    std::weak_ptr<IChannelListener> _mListener;
};

#endif /* IPCCHANNEL_H_ */
