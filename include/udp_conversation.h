#ifndef PACKET_REPLAY_UDP_CONVERSATION_H
#define PACKET_REPLAY_UDP_CONVERSATION_H
    
#include "transport_packet.h"
#include "packet_conversation.h"
#include "target_test_server.h"

namespace packet_replay {

    class UdpConversation : public PacketConversation {
        public:
            UdpConversation& operator=(const UdpConversation&) = delete;

            UdpConversation() = delete;
            
            UdpConversation(const TransportPacket& packet, const TargetTestServer* target_test_server) : PacketConversation(packet, target_test_server) {
            }
            
            void processCapturePacket(const TransportPacket& packet);
            
        protected:
            int socket_;
    };
    
} // namespace packet_replay

#endif
