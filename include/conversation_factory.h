#ifndef PACKET_REPLAY_CONVERSATION_FACTORY_H
#define PACKET_REPLAY_CONVERSATION_FACTORY_H


#include <string>
#include <utility>
#include <vector>

#include "target_test_server.h"
#include "tcp_conversation.h"
#include "transport_packet.h"
#include "udp_conversation.h"

namespace packet_replay {
    std::pair<std::string, TargetTestServer*> tcpIpCreateTargetTestServer(const char* spec);
    std::vector<std::string> tcpIpGetTargetTestServerKeys(const TransportPacket& packet);
    std::string tcpIpGetKey(const TransportPacket& packet);

    /**
     * Class to create Conversation objects
     */
    template <class T>
    class ConversationFactory {
        public:
            /**
             * create a test target server from a string specification
             */
            virtual std::pair<std::string, TargetTestServer*> createTargetTestServer(const char* spec) = 0;

            /**
             * Return a list of value lookup keys for finding the target server
             */
            virtual std::vector<std::string> getTargetTestServerKeys(const TransportPacket& packet) = 0;

            /**
             * Return a key to find a conversation the specified packet belong to
             */
            virtual std::string getKey(const TransportPacket& packet) = 0;

            /**
             * Create a conversation based on a packet and configured test server
             */
            virtual T* createConversation(const TransportPacket& packet, const TargetTestServer* configured_conversation) = 0;
    };

    class TcpConversationFactory : public ConversationFactory<TcpConversation> {
        public:
            std::pair<std::string, TargetTestServer*> createTargetTestServer(const char* spec) {
                return tcpIpCreateTargetTestServer(spec);
            }

            std::vector<std::string> getTargetTestServerKeys(const TransportPacket& packet) {
                return tcpIpGetTargetTestServerKeys(packet);
            }

            std::string getKey(const TransportPacket& packet) {
                return tcpIpGetKey(packet);
            }

            TcpConversation* createConversation(const TransportPacket& packet, const TargetTestServer* test_server) {
                return new TcpConversation(packet, test_server);
            }
    };

    class UdpConversationFactory : public ConversationFactory<UdpConversation> {
        public:
        std::pair<std::string, TargetTestServer*> createTargetTestServer(const char* spec) {
            return tcpIpCreateTargetTestServer(spec);
        }

        std::vector<std::string> getTargetTestServerKeys(const TransportPacket& packet) {
            return tcpIpGetTargetTestServerKeys(packet);
        }

        std::string getKey(const TransportPacket& packet) {
            return tcpIpGetKey(packet);
        }

        UdpConversation* createConversation(const TransportPacket& packet, const TargetTestServer* test_server) {
            return new UdpConversation(packet, test_server);
        }
};

}


#endif