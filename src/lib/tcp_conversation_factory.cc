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
    
    std::pair<std::string, ConfiguredConversation*> TcpConversationFactory::createConfiguredConversation(const char* spec) {
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
        ConfiguredConversation* value =  test_addr.empty() ? nullptr : new ConfiguredConversation(test_addr, test_port);

        return {key, value};
    }

    std::vector<std::string> TcpConversationFactory::getConfiguredConversationKeys(const TransportPacket& packet) {
        Layer3* layer3 = dynamic_cast<Layer3 *>(packet.getLayer(NETWORK));
        TcpLayer* tcpLayer = dynamic_cast<TcpLayer *>(packet.getLayer(TRANSPORT));

        std::vector<std::string> keys;

        keys.push_back(layer3->getSrcAddrStr() + ":" + std::to_string(tcpLayer->getSrcPort()));
        keys.push_back(layer3->getSrcAddrStr());

        return keys;
    }

    std::string TcpConversationFactory::getKey(const TransportPacket& packet) {
        Layer3* layer3 = dynamic_cast<Layer3 *>(packet.getLayer(NETWORK));
        TcpLayer* tcpLayer = dynamic_cast<TcpLayer *>(packet.getLayer(TRANSPORT));

        const void* src_addr = layer3->getSrcAddr();
        const void* dest_addr = layer3->getDestAddr();
        uint16_t src_port = tcpLayer->getSrcPort();
        uint16_t dest_port = tcpLayer->getDestPort();
    
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

    TcpConversation* TcpConversationFactory::createConversation(const TransportPacket& packet, const ConfiguredConversation* configured_conversation) {
        return new TcpConversation(packet, configured_conversation);
    }
} // namespace packet_replay