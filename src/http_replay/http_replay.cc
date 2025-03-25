#include <stdint.h>
#include <unistd.h>

#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <sys/socket.h>

#include <exception>
#include <iostream>
#include <map>

#include "capture.h"
#include "http_replay.h"
#include "http_response_processor.h"
#include "tcp_conversation_store.h"

namespace packet_replay {
    void HttpReplayClient::replay() {

        HttpResponseProcessor expected_processor;
        HttpResponseProcessor test_processor;

        while (!conversation_->actionEmpty()) {
            auto action = conversation_->actionFront();

            switch (action->type_) {
                case TcpConversation::CONNECT: {
                    socket_ = socket(AF_INET, SOCK_STREAM, 0);
                    if (socket_ < 0) {
                        throw std::runtime_error("socket failed: " + std::string(strerror(errno)) + " (" + std::to_string(errno) + ")");
                    }

                    auto [serv_addr, addr_size] = conversation_->getServerAddr();

                    // connect the client socket to server socket
                    if (connect(socket_, (struct sockaddr *) serv_addr, addr_size) != 0) {
                        throw std::runtime_error("connect failed: " + std::string(strerror(errno)) + " (" + std::to_string(errno) + ")");
                    }

                    break;
                }

                case TcpConversation::SEND: {
                    auto nwrite = 0;
                    
                    while (nwrite < action->data_size_) {
                        if (auto n = write(socket_, action->data_ + nwrite, action->data_size_ - nwrite); n > 0) {
                            nwrite += n;
                        } else if (n < 0) {
                            throw std::runtime_error("write failed: " + std::string(strerror(errno)) + " (" + std::to_string(errno) + ")");
                        } else {
                            throw std::runtime_error("write failed: connection closed");
                        }
                    }

                    // this assumes that response for the current request arrives before the another request starts.  Although this isn't strictly a requirement
                    // for http, it should be true almost all of the time.
                    expected_processor.reset();
                    test_processor.reset();

                    break;
                }

                case TcpConversation::RECV: {
                    if (!expected_processor.complete()) {
                        expected_processor.processData(action->data_, action->data_size_);
                    }

                    uint8_t buffer[BUFSIZ];
                                        
                    while (!test_processor.complete()) {
                        if (auto n = read(socket_, buffer, BUFSIZ); n > 0) {
                            test_processor.processData(buffer, n);
                        } else if (n < 0) {
                            throw std::runtime_error("read failed: " + std::string(strerror(errno)) + " (" + std::to_string(errno) + ")");
                        } else {
                            throw std::runtime_error("read failed: connection closed");
                        }    
                    }

                    if (expected_processor.complete()) {
                        if (expected_processor.compare(test_processor)) {
                            std::cout << "detected difference in server response" << std::endl;
                        }
                    }

                    break;
                }

                case TcpConversation::CLOSE:
                    close(socket_);
                    socket_ = -1;
                    break;

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
        packet_replay::TcpConversationStore store;

        int opt;
        while((opt = getopt(argc, argv, "c:")) != -1) {  
            switch(opt)  
            {  
                case 'c':  
                    store.addConfiguredConversation(optarg);
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

        for (auto conv : store.getConversationMap()) {
            packet_replay::HttpReplayClient client(conv.second);

            client.replay();
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }

    return 0;

}

