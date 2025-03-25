#ifndef PACKET_REPLAY_TCP_REPLAY_H
#define PACKET_REPLAY_TCP_REPLAY_H

#include <map>

#include "capture.h"
#include "tcp_conversation.h"

namespace packet_replay {
    /**
     * A class for creating and storing Tcp Conversations
     */
    class TcpConversationStore : public ConversationStore
    {
        protected:
            std::map<std::pair<uint32_t, int>, std::pair<uint32_t, int> *> configured_tcp_conversations_;
            std::map<std::string, packet_replay::TcpConversation*> tcp_conversations_;

        public:
            TcpConversationStore() {
            }

            ~TcpConversationStore() {
                for (auto it = tcp_conversations_.begin(); it != tcp_conversations_.end();)  {
                    delete (*it).second;
                    tcp_conversations_.erase(it++);
                }

                for (auto it = configured_tcp_conversations_.begin(); it != configured_tcp_conversations_.end();)  {
                    delete (*it).second;
                    configured_tcp_conversations_.erase(it++);
                }
            }

            TcpConversationStore(const TcpConversationStore&) = delete;
            TcpConversationStore& operator=(const TcpConversationStore&) = delete;

            /** 
             * Add a conversation to store
             * 
             * @param conversation the conversations specification in the format of <src IP>[:<src port>[:<test IP>[:<test port>]]]
             */
            void addConfiguredConversation(const char* conversation);

            PacketConversation* getConversation(TransportPacket& packet);

            /**
             * Retrieve the map of recorded conversations
             */
            std::map<std::string, packet_replay::TcpConversation*> getConversationMap() {
                return tcp_conversations_;
            }

    };
}

#endif