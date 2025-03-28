#ifndef PACKET_REPLAY_TCP_CONVERSATION_H
#define PACKET_REPLAY_TCP_CONVERSATION_H

#include <stdint.h>
#include <string.h>
#include <netinet/in.h>

#include "target_test_server.h"
#include "packet_conversation.h"

namespace packet_replay {

    /**
     * A recording of a TCP conversation.  The packets are summarized into a list of actions that are stored in a queue for replay.
     */
    class TcpConversation : public PacketConversation {
        public:
            TcpConversation& operator=(const TcpConversation&) = delete;

            TcpConversation() = delete;

            TcpConversation(const TransportPacket& packet, const TargetTestServer* target_test_server) : 
                PacketConversation(packet, target_test_server), cap_tcp_state_(CLOSED), socket_(-1) {
            }

            void processCapturePacket(const TransportPacket& packet);

        protected:
            typedef enum TcpStateEnum {
                SYN_SENT,
                SYN_RECEIVED,
                ESTABLISHED,
                CLOSED // simplication of close handshake
            } TcpState;

            int socket_;
            TcpState cap_tcp_state_;

        private:
            void synSentProcessCapturePacket(const TransportPacket& packet);
            void synRecvProcessCapturePacket(const TransportPacket& packet);
            void estProcessCapturePacket(const TransportPacket& packet);
            void closeProcessCapturePacket(const TransportPacket& packet);

    };
}

#endif