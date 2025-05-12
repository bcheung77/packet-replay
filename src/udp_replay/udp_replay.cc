#include <stdint.h>
#include <unistd.h>

#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <sys/socket.h>

#include <exception>
#include <iostream>
#include <vector>

#include "action.h"
#include "udp_replay.h"
#include "udp_conversation.h"
#include "python_api.h"
#include "util.h"

namespace packet_replay {
    UdpReplayClient::UdpReplayClient(UdpConversation* conversation, PacketValidator* validator) : conversation_(conversation), validator_(validator) {
        socket_ = socket(conversation->getAddressFamily(), SOCK_DGRAM, 0);
        if (socket_ < 0) {
            throw std::runtime_error("socket failed: " + std::string(strerror(errno)) + " (" + std::to_string(errno) + ")");
        }
    }

    void UdpReplayClient::replay() {
        while (!conversation_->actionEmpty()) {
            auto action = conversation_->actionFront();

            switch (action->type_) {
                case Action::Type::SEND: {
                    auto data = action->data();
                    auto nwrite = sendto(socket_, data.data(), data.size(), 0, (const sockaddr*) conversation_->getTestSockAddr(), conversation_->getSockAddrSize());

                    if (nwrite < 0) {
                        throw std::runtime_error("sendto failed: " + std::string(strerror(errno)) + " (" + std::to_string(errno) + ")");
                    }
                    break;
                }

                case Action::Type::RECV: {
                    // TODO: set configurable buffer size
                    auto data_size = action->data().size() + 1024;

                    uint8_t* data = new uint8_t[data_size];

                    // TODO: verify message is really from server
                    auto nread = recvfrom(socket_, data, data_size, 0, nullptr, nullptr);

                    if (nread < 0) {
                        delete[] data;
                        throw std::runtime_error("recvfrom failed: " + std::string(strerror(errno)) + " (" + std::to_string(errno) + ")");
                    }

                    if (!validator_->validate(reinterpret_cast<const uint8_t *>(action->data().data()), action->data().size(), data, nread)) {
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
    std::cerr << "Usage: " << name << "[-c <client spec>] [-k <packet validator spec>] <cap file>" << std::endl;
}

static packet_replay::PacketValidator* parseValidator(const char * spec) {
    std::vector<std::string> tokens = packet_replay::tokenize(spec, ':');

    std::string err = "invalid validator spec '";
    err.append(spec).append("'");

    if (tokens.size() != 3) {
        throw std::invalid_argument(err);
    }

    std::string type_token = tokens[0];
    packet_replay::toLower(type_token);

    if (type_token != "python") {
        throw std::invalid_argument(err);
    }

    return new packet_replay::PythonPacketValidator(tokens[1], tokens[2]);
}

int main(int argc, char* argv[]) {
    try {
        packet_replay::UdpConversationFactory factory;
        packet_replay::TypedConversationStore<packet_replay::UdpConversation> store(factory);

        packet_replay::PacketValidator* validator = new packet_replay::PacketValidator();

        int opt;
        while((opt = getopt(argc, argv, "c:k:")) != -1) {  
            switch(opt)  
            {  
                case 'c':  
                    store.addTargetTestServer(optarg);
                    break;  

                case 'k':
                    validator = parseValidator(optarg);
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
            packet_replay::UdpReplayClient client(conv, validator);

            client.replay();
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }

    return 0;

}

