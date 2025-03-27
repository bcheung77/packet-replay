#ifndef PACKET_REPLAY_CONVERSATION_STORE_H
#define PACKET_REPLAY_CONVERSATION_STORE_H

#include <map>
#include <string>

#include "configured_conversation.h"
#include "conversation_factory.h"
#include "packet_conversation.h"
#include "transport_packet.h"

namespace packet_replay {
    /**
     * An interface to retrieve the conversation a packet belongs to.
     */
    class ConversationStore {
        protected:
            std::map<std::string, ConfiguredConversation*> configured_conversations_;

        public:
            ~ConversationStore() {
                for (auto it = configured_conversations_.begin(); it != configured_conversations_.end();)  {
                    delete (*it).second;
                    configured_conversations_.erase(it++);
                }
            }

            /**
             * Get the conversation a packet belongs to
             * 
             * @return the conversation associated with the packet or nullptr if none found
             */
            virtual PacketConversation* getConversation(TransportPacket& packet) = 0;

            virtual void addConfiguredConversation(const char* spec) = 0;

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

            void addConfiguredConversation(const char* spec);
    };

    template <class T> PacketConversation* TypedConversationStore<T>::getConversation(TransportPacket& packet) {

        packet_replay::TcpConversation* conversation = nullptr;
        bool is_configured = false;

        const std::string conv_key = factory_.getKey(packet);
        if (conversations_.contains(conv_key)) {
            conversation = conversations_[conv_key];
        } else {
            ConfiguredConversation* configured_conversation = nullptr;

            std::vector<std::string> configured_keys = factory_.getConfiguredConversationKeys(packet);

            for (auto key : configured_keys) {
                if (configured_conversations_.contains(key)) {
                    is_configured = true;
                    configured_conversation = configured_conversations_[key];
                    break;
                }
            }
            
            if (is_configured || (configured_conversations_.empty() && conversations_.empty())) {

                conversation = factory_.createConversation(packet, configured_conversation);

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


    template <class T> void TypedConversationStore<T>::addConfiguredConversation(const char* spec) {
        auto configured = factory_.createConfiguredConversation(spec);

        configured_conversations_[configured.first] = configured.second;
    }
}

#endif