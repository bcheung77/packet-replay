#include <string>
#include <stdexcept>
#include <stdint.h>
#include <vector>

#include <arpa/inet.h>
#include <netinet/ip.h>

#include "conversation_factory.h"
#include "tcp_conversation.h"
#include "util.h"

namespace packet_replay {
    static const int NO_PORT = -1;
    
    static std::string generateKey(const std::string& addr1, uint16_t port1, const std::string& addr2, uint16_t port2) {
        const std::string KEY_SEPARATOR = ":";
    
        return addr1 + KEY_SEPARATOR + std::to_string(port1) + KEY_SEPARATOR + addr2 + KEY_SEPARATOR + std::to_string(port2);
    }

    static std::string normalizeIpV6(const char* ipV6_addr) {
        struct in6_addr addr;

        if (!inet_pton(AF_INET6, ipV6_addr, &addr)) {
            throw std::invalid_argument("invalid IPv6 address '" + std::string(ipV6_addr) + "'");
        }

        char addr_str[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, &addr, addr_str, INET6_ADDRSTRLEN);

        return addr_str;
    }

    static std::pair<std::string, TargetTestServer*> parseIpV4Spec(const char* spec) {
        std::string src_addr;
        int src_port = NO_PORT;
        bool has_test_addr = false;
        std::string test_addr;
        int test_port = NO_PORT;

        std::vector<std::string> tokens = tokenize(spec, ':');

        switch (tokens.size())
        {
            case 4:
                try {
                    test_port = htons(stoul(tokens[3]));
                } catch (std::invalid_argument& e) {
                    throw std::invalid_argument("invalid port number '" + tokens[3] + "'");
                }

            case 3: {
                    uint32_t dummy;
                    if (inet_pton(AF_INET, tokens[2].c_str(), &dummy) <= 0) {
                        throw std::invalid_argument("invalid IP address '" + tokens[2] + "'");
                    }
                    test_addr = tokens[2];
                }

            case 2:
                try {
                    src_port = htons(stoul(tokens[1]));
                } catch (std::invalid_argument& e) {
                    throw std::invalid_argument("invalid port number '" + tokens[1] + "'");
                }

            case 1: {
                uint32_t dummy;
                if (inet_pton(AF_INET, tokens[0].c_str(), &dummy) <= 0) {
                        throw std::invalid_argument("invalid IP address '" + tokens[0] + "'");
                    }
                    src_addr = tokens[0];
                }
                break;
            
            default:
                throw std::invalid_argument("invalid conversation specificiation '" + std::string(spec) + "'");
                break;
        }

        std::string key = src_port != NO_PORT ? src_addr + ":" + std::to_string(src_port) : src_addr;
        TargetTestServer* value =  test_addr.empty() ? nullptr : new TargetTestServer(test_addr, htons(test_port));

        return {key, value};
    }

    static std::pair<std::string, TargetTestServer*> parseIpV6Spec(const char* spec) {
        std::string src_addr;
        int src_port = NO_PORT;
        bool has_test_addr = false;
        std::string test_addr;
        int test_port = NO_PORT;

        auto tok_result = token(spec, ']');
        if (tok_result.first != nullptr) {
            src_addr = normalizeIpV6(tok_result.second.substr(1).c_str());
        }

        if (*(tok_result.first + 1) == ':') {
            tok_result = token(tok_result.first + 2, ':');

            if (!tok_result.second.empty()) {
                try {
                    src_port = htons(stoul(tok_result.second));
                } catch (std::invalid_argument& e) {
                    throw std::invalid_argument("invalid port number '" + tok_result.second + "'");
                }
            }
        }

        if (tok_result.first != nullptr) {
            tok_result = token(tok_result.first + 1, ']');

            if (!tok_result.second.empty()) {
                if (tok_result.first == nullptr) {
                    test_addr = normalizeIpV6(tok_result.second.c_str());
                } else {
                    if (tok_result.second.at(0) != '[') {
                        throw std::invalid_argument("invalid spec '" + std::string(spec) + "'");
                    }

                    test_addr = normalizeIpV6(tok_result.second.substr(1).c_str());

                    if (*(tok_result.first + 1) != ':') {
                        throw std::invalid_argument("invalid spec '" + std::string(spec) + "'");
                    }

                    tok_result = token(tok_result.first + 2, ':');

                    if (tok_result.first != nullptr) {
                        throw std::invalid_argument("invalid spec '" + std::string(spec) + "'");                        
                    } else if (!tok_result.second.empty()) {
                        try {
                            test_port = htons(stoul(tok_result.second));
                        } catch (std::invalid_argument& e) {
                            throw std::invalid_argument("invalid port number '" + tok_result.second + "'");
                        }        
                    }
                }
            }
        }

        std::string key = src_port != NO_PORT ? src_addr + ":" + std::to_string(src_port) : src_addr;
        TargetTestServer* value =  test_addr.empty() ? nullptr : new TargetTestServer(test_addr, htons(test_port));

        return {key, value};
    }


    std::pair<std::string, TargetTestServer*> tcpIpCreateTargetTestServer(const char* spec) {
        if (spec[0] == '[') {
            return parseIpV6Spec(spec);
        }

        return parseIpV4Spec(spec);
    }

    std::vector<std::string> tcpIpGetTargetTestServerKeys(const TransportPacket& packet) {
        Layer3* layer3 = dynamic_cast<Layer3 *>(packet.getLayer(NETWORK));
        Layer4* layer4 = dynamic_cast<Layer4 *>(packet.getLayer(TRANSPORT));

        std::vector<std::string> keys;

        keys.push_back(layer3->getSrcAddrStr() + ":" + std::to_string(layer4->getSrcPort()));
        keys.push_back(layer3->getSrcAddrStr());

        return keys;
    }

    std::string tcpIpGetKey(const TransportPacket& packet) {
        Layer3* layer3 = dynamic_cast<Layer3 *>(packet.getLayer(NETWORK));
        Layer4* layer4 = dynamic_cast<Layer4 *>(packet.getLayer(TRANSPORT));

        const void* src_addr = layer3->getSrcAddr();
        const void* dest_addr = layer3->getDestAddr();
        uint16_t src_port = layer4->getSrcPort();
        uint16_t dest_port = layer4->getDestPort();
    
        auto src_addr_str = bytes_to_hex_string(static_cast<const uint8_t *>(src_addr), layer3->getAddrSize());
        auto dest_addr_str = bytes_to_hex_string(static_cast<const uint8_t *>(dest_addr), layer3->getAddrSize());

        auto comp = memcmp(src_addr, dest_addr, layer3->getAddrSize());
        if (comp == 0) {
            if (src_port < dest_port) {
                return generateKey(src_addr_str, src_port, dest_addr_str, dest_port);
    
            } else {
                return generateKey(dest_addr_str, dest_port, src_addr_str, src_port);
            }
        } else if (comp < 0) {
            return generateKey(src_addr_str, src_port, dest_addr_str, dest_port);
        } else {
            return generateKey(dest_addr_str, dest_port, src_addr_str, src_port);
        }
    }
} // namespace packet_replay