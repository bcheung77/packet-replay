#ifndef PACKET_REPLAY_CONVERSATION_STORE_H
#define PACKET_REPLAY_CONVERSATION_STORE_H

#include <map>
#include <string>

#include "target_test_server.h"
#include "conversation_factory.h"
#include "packet_conversation.h"
#include "transport_packet.h"

namespace packet_replay {
    /**
     * An interface to retrieve the conversation a packet belongs to.
     */
    class ConversationStore {
        protected:
            std::map<std::string, TargetTestServer*> test_servers_;

        public:
            ~ConversationStore() {
                for (auto it = test_servers_.begin(); it != test_servers_.end();)  {
                    delete (*it).second;
                    test_servers_.erase(it++);
                }
            }

            /**
             * Get the conversation a packet belongs to
             * 
             * @return the conversation associated with the packet or nullptr if none found
             */
            virtual PacketConversation* getConversation(TransportPacket& packet) = 0;

            virtual void addTargetTestServer(const char* spec) = 0;

    };

    /**
     * A conversation store that holds a specific conversation type.
     */
    template <class T>
    class TypedConversationStore : public ConversationStore {
        protected:
            ConversationFactory<T>& factory_;
            std::map<std::string, T*> conversations_;

        public:
            TypedConversationStore(ConversationFactory<T>& factory) : factory_(factory) {
            }

            ~TypedConversationStore() {
                for (auto it = conversations_.begin(); it != conversations_.end();)  {
                    delete (*it).second;
                    conversations_.erase(it++);
                }
            }

            /**
             * Retrieve the map of recorded conversations
             */
            std::vector<T*> getConversations();

            PacketConversation* getConversation(TransportPacket& packet);

            void addTargetTestServer(const char* spec);
    };

    template <class T> PacketConversation* TypedConversationStore<T>::getConversation(TransportPacket& packet) {

        T* conversation = nullptr;
        bool is_configured = false;

        const std::string conv_key = factory_.getKey(packet);
        if (conversations_.contains(conv_key)) {
            conversation = conversations_[conv_key];
        } else {
            TargetTestServer* test_server = nullptr;

            std::vector<std::string> configured_keys = factory_.getTargetTestServerKeys(packet);

            for (auto key : configured_keys) {
                if (test_servers_.contains(key)) {
                    is_configured = true;
                    test_server = test_servers_[key];
                    break;
                }
            }
            
            if (is_configured || (test_servers_.empty() && conversations_.empty())) {

                conversation = factory_.createConversation(packet, test_server);

                conversations_[conv_key] = conversation;
            } 
        }

        return conversation;
    }

    template <class T> std::vector<T*> TypedConversationStore<T>::getConversations() {
        std::vector<T*> conversations;

        for (auto pair : conversations_) {
            conversations.push_back(pair.second);
        }

        return conversations;
    }


    template <class T> void TypedConversationStore<T>::addTargetTestServer(const char* spec) {
        auto configured = factory_.createTargetTestServer(spec);

        test_servers_[configured.first] = configured.second;
    }
}

#endif