#ifndef PACKET_REPLAY_CONVERSATION_SERIALIZER
#define PACKET_REPLAY_CONVERSATION_SERIALIZER

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "action.h"
#include "packet_conversation.h"

namespace packet_replay {
    class ConversationSerializer {
        public:
            enum class DataSpecifier {
                AUTO_DETECT,
                BINARY,
                TEXT
            };

            ConversationSerializer() {
            }

            ~ConversationSerializer() {
            }

            void write(std::ostream& output, const PacketConversation* conversation);

            std::unique_ptr<PacketConversation> read(std::istream& input);

        private:
            DataSpecifier data_type_ = DataSpecifier::AUTO_DETECT;
            std::string data_start_tag_ = "<#DATA_START#>";
            std::string data_end_tag_ = "<#DATA_END#>";
            std::string subPrefix_ = "${";
            std::string subSuffix_ = "}";

            void write_action(std::ostream& output, const Action* conversation);
            void read_actions(std::istream& input, PacketConversation& conversation);
            std::vector<char> read_action_data(std::istream& input);
            Action* create_action(Action::Type type, std::vector<char>&& data);
    };    
} // namespace packet_replay


#endif