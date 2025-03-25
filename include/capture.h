#ifndef PACKET_REPLAY_CAPTURE_H
#define PACKET_REPLAY_CAPTURE_H

#include "packet_conversation.h"
#include "transport_packet.h"

namespace packet_replay {
    /**
     * An interface to retrieve the conversation a packet belongs to.
     */
    class ConversationStore {
        public:
            /**
             * Get the conversation a packet belongs to
             * 
             * @return the conversation associated with the packet or nullptr if none found
             */
            virtual PacketConversation* getConversation(TransportPacket& packet) = 0;
    };

    /**
     * Class responsible for loading, parsing, and processing a PCAP capture file
     */
    class Capture {
        private:
            ConversationStore& conversation_store_;
            unsigned char datalink_type_;

        public:
            /**
             * Constructor
             * 
             * @param conversation_store the store responsible for creating and mapping conversations based on the packets
             */
            Capture(ConversationStore& conversation_store) : conversation_store_(conversation_store) {
            }

            /**
             * Handle a pcap packet.
             * 
             * @param h the pcap packet header
             * @param bytes the packet data
             */
            void packetHandler(const struct pcap_pkthdr *h, const u_char *bytes);

            /**
             * Load a PCAP capture file and dissect into conversations
             */
            void load(const char* capture_file);
    };
}

#endif