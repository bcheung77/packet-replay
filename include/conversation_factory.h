#ifndef PACKET_REPLAY_CONVERSATION_FACTORY_H
#define PACKET_REPLAY_CONVERSATION_FACTORY_H


#include <string>
#include <utility>
#include <vector>

#include "target_test_server.h"
#include "tcp_conversation.h"
#include "transport_packet.h"

namespace packet_replay {
    /**
     * Class to create Conversation objects
     */
    template <typename T>
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
            std::pair<std::string, TargetTestServer*> createTargetTestServer(const char* spec);
            std::vector<std::string> getTargetTestServerKeys(const TransportPacket& packet);
            std::string getKey(const TransportPacket& packet);
            TcpConversation* createConversation(const TransportPacket& packet, const TargetTestServer* configured_conversation);
    };
}


#endif