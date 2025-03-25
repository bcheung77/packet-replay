#ifndef PACKET_REPLAY_PACKET_CONVERSATION_H
#define PACKET_REPLAY_PACKET_CONVERSATION_H

#include "transport_packet.h"

namespace packet_replay {
    /**
     * Base class for a network conversation
     */
    class PacketConversation {
        public:
            /**
             * Insert a packet into the conversation
             */
            virtual void processCapturePacket(const TransportPacket& packet) = 0;
    };
}

#endif