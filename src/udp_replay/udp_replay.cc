#include <stdint.h>
#include <unistd.h>

#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <sys/socket.h>

#include <exception>
#include <iostream>
#include <map>

#include "udp_replay.h"
#include "udp_conversation.h"

namespace packet_replay {
    UdpReplayClient::UdpReplayClient(UdpConversation* conversation) : conversation_(conversation) {
        socket_ = socket(conversation->getAddressFamily(), SOCK_DGRAM, 0);
        if (socket_ < 0) {
            throw std::runtime_error("socket failed: " + std::string(strerror(errno)) + " (" + std::to_string(errno) + ")");
        }
    }

    void UdpReplayClient::replay() {
        while (!conversation_->actionEmpty()) {
            auto action = conversation_->actionFront();

            switch (action->type_) {
                case PacketConversation::SEND: {
                    auto nwrite = sendto(socket_, action->data_, action->data_size_, 0, (const sockaddr*) conversation_->getTestSockAddr(), conversation_->getSockAddrSize());

                    if (nwrite < 0) {
                        throw std::runtime_error("sendto failed: " + std::string(strerror(errno)) + " (" + std::to_string(errno) + ")");
                    }
                    break;
                }

                case PacketConversation::RECV: {
                    uint8_t* data = new uint8_t[action->data_size_];

                    // TODO: verify message is really froms server
                    auto nread = recvfrom(socket_, data, action->data_size_, 0, nullptr, nullptr);

                    if (nread < 0) {
                        delete[] data;
                        throw std::runtime_error("recvfrom failed: " + std::string(strerror(errno)) + " (" + std::to_string(errno) + ")");
                    }

                    if (nread != action->data_size_ || memcmp(data, action->data_, action->data_size_) != 0) {
                        std::cout << "detected difference in server response" << std::endl;
                    }

                    delete[] data;

                    break;
                }
            }

            conversation_->actionPop();

        }
    }
}

static void printUsage(const char* name) {
    std::cerr << "Usage: " << name << "[-c <client spec>] <cap file>" << std::endl;
}

int main(int argc, char* argv[]) {

    try {
        packet_replay::UdpConversationFactory factory;
        packet_replay::TypedConversationStore<packet_replay::UdpConversation> store(factory);

        int opt;
        while((opt = getopt(argc, argv, "c:")) != -1) {  
            switch(opt)  
            {  
                case 'c':  
                    store.addTargetTestServer(optarg);
                    break;  

                default:
                    printUsage(argv[0]);
                    return -1;
            }  
        }  
        
        if (optind + 1 != argc) {
            printUsage(argv[0]);
            return -1;
        }

        packet_replay::Capture capture(store);

        // store.addConfiguredConversation("127.0.0.1:63596");
        // store.addConfiguredConversation("192.168.1.72:64501");
        // store.addConfiguredConversation("127.0.0.1");

        capture.load(argv[optind]);

        for (auto conv : store.getConversations()) {
            packet_replay::UdpReplayClient client(conv);

            client.replay();
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }

    return 0;

}

