/*
 * IPCChannel.cpp
 *
 *  Created on: 2016年12月21日
 *      Author: hamersun
 */

#include "IPCChannel.h"
#include "IChannelListener.h"
#include "uds.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <thread>
#include <vector>

static const unsigned int MAX_BUF_SIZE = (8<<10); // 8K bytes
static std::thread comm_thread;

void remove_peer(std::vector<PeerInfo>&list, int target_fd) {
    for (auto it = list.begin(); it < list.end(); it++) {
        if (it->fd == target_fd) {
            list.erase(it);
        }
    }
}

PeerInfo* find_peer(std::vector<PeerInfo>&list, int target_fd) {
    for (auto it = list.begin(); it < list.end(); it++) {
        if (it->fd == target_fd) {
            return &(*it);
        }
    }
    return nullptr;
}

IPCChannel::IPCChannel(ipc_role role)
: _mRole(role)
, _mStop(true)
, _mFd(-1)
{
    // TODO Auto-generated constructor stub
}

IPCChannel::~IPCChannel()
{
    _close();
}

IPCChannel::pointer IPCChannel::create(ipc_role role)
{
    IPCChannel::pointer channel(new IPCChannel(role));
    return channel;
}

ipc_status IPCChannel::setup(const char *comm_file)
{
    ipc_status ret = IPC_SUCCESS;
    _mCommAddress = comm_file;
    if (SERVER == _mRole) {
        if (_setup_server() < 0) {
            ret = IPC_FAILED_SETUP_LISTEN;
        }
    } else {
        if (_setup_client() < 0) {
            ret = IPC_FAILED_CONNECTION;
        }
    }
    return ret;
}

ipc_status IPCChannel::teardown()
{
    if (!_mStop) {
        _mStop = true;
    }
    if (comm_thread.joinable())
        comm_thread.join();
    return IPC_SUCCESS;
}

void IPCChannel::setChannelListener(std::shared_ptr<IChannelListener> listener)
{
    _mListener = listener;
}

int IPCChannel::sendData(struct PeerInfo &peer, const char *data, const int32_t data_size)
{
    int ret = send_data(peer.fd, data, data_size);
    if (ret < 0) {
        fprintf(stderr, "send data to peer %d failed: %d(%s)\n", peer.uid, errno, strerror(errno));
    }
    return ret;
}

ipc_role IPCChannel::role()
{
    return _mRole;
}

// ================ private ===================
int IPCChannel::_setup_server()
{
    int ret;
    if ((ret = serv_listen(_mCommAddress.c_str())) < 0)
        return ret;
    _mFd = ret;
    comm_thread = std::thread(&IPCChannel::_handleCommunicationAsServer, shared_from_this());
    return 0;
}

int IPCChannel::_setup_client()
{
    int ret;
    if ((ret = connect_server(_mCommAddress.c_str())) < 0)
        return ret;
    _mFd = ret;
    comm_thread = std::thread(&IPCChannel::_handleCommunicationAsClient, shared_from_this());
    return 0;
}

void IPCChannel::_close()
{
    if (SERVER == _mRole) {
        serv_teardown(_mFd);
    } else {
        close_connection(_mFd);
    }
}

void IPCChannel:: _handleCommunicationAsClient()
{
    fd_set read_fds; // for select()
    int fd_max;
    struct timeval timeout;
    int result = 0;
    PeerInfo peerInfo;
    std::vector<PeerInfo> peer_list;
    char buf[MAX_BUF_SIZE];

    FD_ZERO(&read_fds);

    // get peer info
    peerInfo.uid = get_peer_uid(_mFd);
    peerInfo.fd = _mFd;
    fprintf(stdout, "peer uid: %d\n", peerInfo.uid);
    peer_list.push_back(peerInfo);
    {
        auto listener = _mListener.lock();
        if (listener) {
            listener->OnConnected(peerInfo);
        }
    }

    fd_max = _mFd;
    _mStop = false;
    while (!_mStop) {
        FD_ZERO(&read_fds);
        FD_SET(_mFd, &read_fds);

        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        if ((result = select(fd_max+1, &read_fds, NULL, NULL, &timeout)) == 0) {
            // timeout
            continue;
        } else if (result < 0) {
            // something wrong
            fprintf(stderr, "select failed: %d(%s)\n", errno, strerror(errno));
            _mStop = true;
            continue;
        }

        if (FD_ISSET(_mFd, &read_fds)) {
            if (_recv_data(peer_list, _mFd, buf, MAX_BUF_SIZE) <= 0) {
                close_connection(_mFd);
                remove_peer(peer_list, _mFd);
                _mStop = true;
            }
        }
    } // end of while loop
    auto listener = _mListener.lock();
    if (listener) {
        listener->OnStop();
    }
}

void IPCChannel:: _handleCommunicationAsServer()
{
    fd_set master; // master file descriptors
    fd_set read_fds; // for select()
    int fd_max;
    struct timeval timeout;
    int result = 0;
    PeerInfo peerInfo;
    std::vector<PeerInfo> peer_list;
    char buf[MAX_BUF_SIZE];

    FD_ZERO(&master);
    FD_ZERO(&read_fds);

    FD_SET(_mFd, &master);
    fd_max = _mFd;
    _mStop = false;
    while (!_mStop) {
        read_fds = master;

        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        if ((result = select(fd_max+1, &read_fds, NULL, NULL, &timeout)) == 0) {
            // timeout
            continue;
        } else if (result < 0) {
            // something wrong
            fprintf(stderr, "select failed: %d(%s)\n", errno, strerror(errno));
            _mStop = true;
            continue;
        }

        for (int fd = 0; fd<= fd_max; ++fd) {
            if (FD_ISSET(fd, &read_fds)) {
                if (fd == _mFd) {
                    // handle new connection
                    if (_handle_new_connection(peerInfo) == 0) {
                        peer_list.push_back(peerInfo);
                        FD_SET(peerInfo.fd, &master); // add to master set
                        if (peerInfo.fd > fd_max) {
                            fd_max = peerInfo.fd;
                        }
                    }
                } else {
                    // handle data from client
                    fprintf(stdout, "start to recv data\n");
                    if (_recv_data(peer_list, fd, buf, MAX_BUF_SIZE) <= 0) {
                        // got error or connection closed by client
                        close_connection(fd);
                        FD_CLR(fd, &master); // remove from master
                        remove_peer(peer_list, fd); // remove from list
                    }
                } // end of handle data from client
            } // end of FD_ISSET
        } // end of fds loop
    } // end of while loop
    auto listener = _mListener.lock();
    if (listener) {
        listener->OnStop();
    }
}

int IPCChannel::_handle_new_connection(PeerInfo &info)
{
    int ret = 0;
    uid_t uid_peer;
    int fd_new = serv_accept(_mFd, &uid_peer);
    if (fd_new < 0) {
        fprintf(stderr, "accept failed: %d(%s)\n", errno, strerror(errno));
        ret = -1;
    } else {
        // add new peer info
        info.uid = uid_peer;
        info.fd = fd_new;
        auto listener = _mListener.lock();
        if (listener) {
            listener->OnConnected(info);
        }
    }
    return ret;
}

int IPCChannel::_recv_data(std::vector<PeerInfo> &list, int fd, char *buf, const int max_size)
{
    if (buf == nullptr) {
        return 1; // prevent from removing peer from list
    }
    int nbytes = 0;
    auto info = find_peer(list, fd);
    memset(buf, 0, max_size);
    if ((nbytes = recv_data(fd, buf, max_size)) <= 0) {
        // got error or connection closed by client
        if (nbytes == 0) {
            // close connection
            if (info != nullptr) {
                fprintf(stdout, "peer %d disconnection\n", info->uid);
                auto listener = _mListener.lock();
                if (listener) {
                    listener->OnDisconnected(*info);
                }
            }
        } else {
            // recv failed
            fprintf(stderr, "recv from peer %d failed: %d(%s)\n", (info!=nullptr)?info->uid:0, errno, strerror(errno));
        } // end of nbytes check
    } else {
        // recv data from peer
        auto listener = _mListener.lock();
        if (listener) {
            listener->OnDataReceived(*info, buf, nbytes);
        }
    } // end of recv_data
    return nbytes;
}

