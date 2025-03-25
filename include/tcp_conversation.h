#ifndef PACKET_REPLAY_TCP_CONVERSATION_H
#define PACKET_REPLAY_TCP_CONVERSATION_H

#include <queue>

#include <stdint.h>
#include <string.h>
#include <netinet/in.h>

#include "packet_conversation.h"

namespace packet_replay {

    /**
     * A recording of a TCP conversation.  The packets are summarized into a list of actions that are stored in a queue for replay.
     */
    class TcpConversation : public PacketConversation {
        public:
            typedef enum {
                CONNECT,
                SEND,
                RECV,
                CLOSE
            } ActionType;

            /**
             * Describes an action that took place in the capture.
             */
            typedef struct action {
                ActionType type_;
                uint8_t* data_;
                int data_size_;

                action(ActionType type) : type_(type), data_(nullptr) {
                }

                action() : data_(nullptr) {                   
                }

                ~action() {
                    delete data_;
                }
            } Action;

            TcpConversation& operator=(const TcpConversation&) = delete;

            TcpConversation() = delete;

            virtual ~TcpConversation() {
                while (!action_queue_.empty()) {
                    delete action_queue_.front();
                    action_queue_.pop();
                }
            }

            virtual std::pair<void *, int> getServerAddr() = 0;


            /**
             * The current action.
             */
            Action* actionFront() {
                return action_queue_.front();
            }

            bool actionEmpty() {
                return action_queue_.empty();
            }

            /**
             * remove the current action
             */
            void actionPop() {
                delete action_queue_.front();
                action_queue_.pop();
            }

            /**
             * Record the specified packet into this conversation.
             */
            virtual void processCapturePacket(const TransportPacket& packet) = 0;

        protected:
            typedef enum TcpStateEnum {
                SYN_SENT,
                SYN_RECEIVED,
                ESTABLISHED,
                CLOSED // simplication of close handshake
            } TcpState;


            TcpConversation(uint16_t cap_src_port, uint16_t cap_dest_port, uint16_t test_dest_port) 
                : cap_src_port_{cap_src_port}, cap_dest_port_{cap_dest_port}, test_dest_port_{test_dest_port}, 
                    capTcpState_(CLOSED), socket_(-1) {
            }

            TcpConversation(uint16_t cap_src_port, uint16_t cap_dest_port) 
                : TcpConversation(cap_src_port, cap_dest_port, cap_dest_port) {
            } 

            uint16_t cap_dest_port_;
            uint16_t cap_src_port_;
            uint16_t test_dest_port_;
            int socket_;
            TcpState capTcpState_;

            std::queue<Action *> action_queue_;            
    };

    /**
     * TCP conversation over IPv4
     */
    class IpV4TcpConversation : public TcpConversation {
        private:
            uint32_t cap_src_ip_;
            uint32_t cap_dest_ip_;
            uint32_t test_dest_ip_;
            struct sockaddr_in serv_addr_;

            void synSentProcessCapturePacket(const TransportPacket& packet);
            void synRecvProcessCapturePacket(const TransportPacket& packet);
            void estProcessCapturePacket(const TransportPacket& packet);
            void closeProcessCapturePacket(const TransportPacket& packet);

        public:
            IpV4TcpConversation(uint32_t cap_src_ip, uint16_t cap_src_port, uint32_t cap_dest_ip, uint16_t cap_dest_port, uint32_t test_dest_ip, uint16_t test_dest_port) 
                : TcpConversation(cap_src_port, cap_dest_port, test_dest_port), cap_src_ip_{cap_src_ip}, cap_dest_ip_{cap_dest_ip}, test_dest_ip_{test_dest_ip} {

                memset(&serv_addr_, 0, sizeof(serv_addr_));

                // assign IP, PORT
                serv_addr_.sin_family = AF_INET;
                serv_addr_.sin_addr.s_addr = test_dest_ip_;
                serv_addr_.sin_port = test_dest_port_;
            }

            IpV4TcpConversation(uint32_t cap_src_ip, uint16_t cap_src_port, uint32_t cap_dest_ip, uint16_t cap_dest_port) 
                : IpV4TcpConversation(cap_src_ip, cap_src_port, cap_dest_ip, cap_dest_port, cap_dest_ip, cap_dest_port) {
            }

            std::pair<void *, int> getServerAddr() {
                return {&serv_addr_, sizeof(serv_addr_)};
            }

            void processCapturePacket(const TransportPacket& packet);
    };
}

#endif