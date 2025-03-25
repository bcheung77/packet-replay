#include <string>
#include <sstream>
#include <stdexcept>
#include <stdint.h>

#include <arpa/inet.h>
#include <netinet/ip.h>

#include "tcp_conversation_store.h"

namespace packet_replay {
    static const int NO_PORT = -1;
    
    static std::string generateKey(uint32_t ip1, uint16_t port1, uint32_t ip2, uint16_t port2) {
        const std::string KEY_SEPARATOR = ":";
    
        return std::to_string(ip1) + KEY_SEPARATOR + std::to_string(port1) + KEY_SEPARATOR + 
            std::to_string(ip2) + KEY_SEPARATOR + std::to_string(port2);
    }
    
    static std::string getTcpKey(const TransportPacket& packet) {
        IpLayer* ipLayer = dynamic_cast<IpLayer *>(packet.getLayer(NETWORK));
        TcpLayer* tcpLayer = dynamic_cast<TcpLayer *>(packet.getLayer(TRANSPORT));

        uint32_t src_ip = ipLayer->getSrcIp();
        uint32_t dest_ip = ipLayer->getDestIp();
        uint16_t src_port = tcpLayer->getSrcPort();
        uint16_t dest_port = tcpLayer->getDestPort();
    
        if (src_ip == dest_ip) {
            if (src_port < dest_port) {
                return generateKey(src_ip, src_port, dest_ip, dest_port);
    
            } else {
                return generateKey(dest_ip, dest_port, src_ip, src_port);
            }
        } else if (src_ip < dest_ip) {
            return generateKey(src_ip, src_port, dest_ip, dest_port);
        } else {
            return generateKey(dest_ip, dest_port, src_ip, src_port);
        }
    }

    void TcpConversationStore::addConfiguredConversation(const char* conversation) {
        std::stringstream str_stream{std::string(conversation)};

        std::string token;
        uint32_t src_ip;
        int src_port = NO_PORT;
        bool has_test_ip = false;
        uint32_t test_ip;
        int test_port = NO_PORT;

        if (!std::getline(str_stream, token, ':') || token.empty()) {
            throw std::invalid_argument("invalid conversation specificiation '" + std::string(conversation) + "'");
        }

        if (inet_pton(AF_INET, token.c_str(), &src_ip) <= 0) {
            throw std::invalid_argument("invalid IP address '" + token + "'");
        }

        if (std::getline(str_stream, token, ':')) {
            try {
                src_port = htons(stoul(token));
            } catch (std::invalid_argument& e) {
                throw std::invalid_argument("invalid port number '" + token + "'");
            }
        }

        if (std::getline(str_stream, token, ':')) {
            if (inet_pton(AF_INET, token.c_str(), &test_ip) <= 0) {
                throw std::invalid_argument("invalid IP address '" + token + "'");
            }

            has_test_ip = true;
        }

        if (std::getline(str_stream, token, ':')) {
            try {
                test_port = htons(stoul(token));
            } catch (std::invalid_argument& e) {
                throw std::invalid_argument("invalid port number '" + token + "'");
            }
        }

        std::pair<uint32_t, int>* test_pair = nullptr;

        if (has_test_ip) {
            test_pair = new std::pair<uint32_t, int>(test_ip, test_port);
        }

        configured_tcp_conversations_[{src_ip, src_port}] = test_pair;
    }

    PacketConversation* TcpConversationStore::getConversation(TransportPacket& packet) {

        packet_replay::TcpConversation* conversation = nullptr;

        if (packet.isLayer(NETWORK, PROTO_IP) && packet.isLayer(TRANSPORT, PROTO_TCP)) {
            IpLayer* ipLayer = dynamic_cast<IpLayer *>(packet.getLayer(NETWORK));
            TcpLayer* tcpLayer = dynamic_cast<TcpLayer *>(packet.getLayer(TRANSPORT));

            const std::string key = getTcpKey(packet);
            if (auto found = tcp_conversations_.find(key); found != tcp_conversations_.end()) {
                conversation = found->second;
            } else {
                std::pair<uint32_t, int>* test_spec = nullptr;
                auto configured_found = configured_tcp_conversations_.contains({ipLayer->getSrcIp(), tcpLayer->getSrcPort()});

                if (configured_found) {
                    test_spec = configured_tcp_conversations_[{ipLayer->getSrcIp(), tcpLayer->getSrcPort()}];
                } else if (configured_found = configured_tcp_conversations_.contains({ipLayer->getSrcIp(), NO_PORT})) {
                    test_spec = configured_tcp_conversations_[{ipLayer->getSrcIp(), NO_PORT}];
                }

                if (configured_found) {
                    if (test_spec) {
                        conversation = new packet_replay::IpV4TcpConversation(ipLayer->getSrcIp(), tcpLayer->getSrcPort(), ipLayer->getDestIp(), tcpLayer->getDestPort(), 
                                                test_spec->first, test_spec->second == NO_PORT ? tcpLayer->getDestPort() : test_spec->second);
                    } else {
                        conversation = new packet_replay::IpV4TcpConversation(ipLayer->getSrcIp(), tcpLayer->getSrcPort(), ipLayer->getDestIp(), tcpLayer->getDestPort());
                    }

                } else if (configured_tcp_conversations_.empty() && tcp_conversations_.empty()) {
                    conversation = new packet_replay::IpV4TcpConversation(ipLayer->getSrcIp(), tcpLayer->getSrcPort(), ipLayer->getDestIp(), tcpLayer->getDestPort());
                }

                if (conversation) {
                    const auto [it, success] = tcp_conversations_.insert({key, conversation});

                    if (!success) {
                        throw std::runtime_error("internal error: failed to insert TCP client");
                    }
                }
            }
        }

        return conversation;
    }

}
