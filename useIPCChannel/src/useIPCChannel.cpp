//============================================================================
// Name        : useIPCChannel.cpp
// Author      : hamer
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include <string>
#include <algorithm>
#include <memory>
#include <unistd.h>
#include "IChannelListener.h"
#include "IPCChannel.h"
#include "uds.h"
using namespace std;

#define SOCK_FILE "/data/tmp/my_uds_file"

bool bStop = true;
PeerInfo info;
class MyChannelListener : public IChannelListener {
public:
    void setChannel(std::shared_ptr<IPCChannel> channel) {
        mChannel = channel;
    }
    void OnConnected(struct PeerInfo peer) override {
        std::cout << __FUNCTION__ << " peer: uid = " << peer.uid << ", fd = " << peer.fd << std::endl;
        info = peer;
    }
    void OnDisconnected(struct PeerInfo peer) override {
        std::cout << __FUNCTION__ << " peer: uid = " << peer.uid << ", fd = " << peer.fd << std::endl;
    }
    void OnStop() override {
        bStop = true;
    }
    void OnDataReceived(struct PeerInfo peer, char *data, const int32_t data_size) override {
        std::cout << __FUNCTION__ << " peer: uid = " << peer.uid << ", fd = " << peer.fd << std::endl;
        std::cout << "\t data size: " << data_size << std::endl;
        std::cout << "\t data: " << data << std::endl;
        auto channel = mChannel.lock();
        if (channel && channel->role() == SERVER) {
            std::string response("echo: ");
            response += data;
            channel->sendData(peer, response.c_str(), response.length());
        }
    }

private:
    std::weak_ptr<IPCChannel> mChannel;
};

void usage(const char *me) {
    fprintf(stderr, "usage: %s [-c] as a client\n"
                                    "\t\t[-s] as a server\n"
                                    "\t\t[-p] communication path\n",
                                    me
                    );
    exit(1);
}

bool stdinAvailable() {
    struct timeval tv;
    fd_set fds;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
    return (FD_ISSET(0, &fds));
}

int main(int argc, char *argv[]) {
    cout << "udsIPC" << endl; // prints udsIPC
    ipc_role role = SERVER;
    std::string comm_path;
    int res;
    while ((res = getopt(argc, argv, "scp:?h")) >= 0) {
        switch(res) {
        case 'c':
            role = CLIENT;
            break;
        case 's':
            role = SERVER;
            break;
        case 'p':
            comm_path = optarg;
            break;
        case 'h':
        case '?':
        default:
            usage(argv[0]);
        }
    }
    if (comm_path.length() == 0) {
        comm_path = SOCK_FILE;
    }
    const char *strRole;
    if (SERVER == role)
        strRole = "sever";
    else strRole = "client";
    std::cout << "as " << strRole << " with path " << comm_path << std::endl;
    std::shared_ptr<MyChannelListener> listener = std::make_shared<MyChannelListener>();
    auto channel = IPCChannel::create(role);
    channel->setChannelListener(listener);
    listener->setChannel(channel);
    ipc_status ret = channel->setup(comm_path.c_str());
    if (IPC_SUCCESS != ret) {
        return -1;
    }

    bStop = false;
    std::string cmd_write("write");
    do {
        std::string line;
       if (!stdinAvailable()) {
           continue;
       }
        getline(cin, line);
        if (line.compare("stop") == 0) {
            bStop = true;
            break;
        } else {
            auto res = std::mismatch(cmd_write.begin(), cmd_write.end(), line.begin());
            if (res.first == cmd_write.end()) {
                // write command
                channel->sendData(info, line.c_str() + cmd_write.length(), line.length() - cmd_write.length());
            }
        }
    } while(!bStop);

    std::cout << "stop communication" << std::endl;
     channel->teardown();
    return 0;
}
