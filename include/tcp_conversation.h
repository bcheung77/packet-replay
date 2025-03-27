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

            TcpConversation(const TransportPacket& packet, const TargetTestServer* configured_conversation);

            virtual ~TcpConversation() {
                while (!action_queue_.empty()) {
                    delete action_queue_.front();
                    action_queue_.pop();
                }

                delete[] cap_src_addr_;
                delete[] cap_dest_addr_;
                delete[] test_dest_addr_;
                delete[] test_sock_addr_;
            }

            void processCapturePacket(const TransportPacket& packet);

            int getAddressFamily() {
                return addr_family_;
            }

            int getSockAddrSize() {
                return sock_addr_size_;
            }

            void* getTestSockAddr() {
                return test_sock_addr_;
            }

        protected:
            typedef enum TcpStateEnum {
                SYN_SENT,
                SYN_RECEIVED,
                ESTABLISHED,
                CLOSED // simplication of close handshake
            } TcpState;

            int addr_family_;
            int addr_size_;
            int sock_addr_size_;
            uint8_t* test_sock_addr_ = nullptr;
            uint8_t* cap_src_addr_ = nullptr;
            uint8_t* cap_dest_addr_ = nullptr;
            uint8_t* test_dest_addr_ = nullptr;

            uint16_t cap_dest_port_;
            uint16_t cap_src_port_;
            uint16_t test_dest_port_;
            int socket_;
            TcpState capTcpState_;

        private:
            void synSentProcessCapturePacket(const TransportPacket& packet);
            void synRecvProcessCapturePacket(const TransportPacket& packet);
            void estProcessCapturePacket(const TransportPacket& packet);
            void closeProcessCapturePacket(const TransportPacket& packet);

    };
}

#endif