#ifndef PACKET_REPLAY_PACKET_CONVERSATION_H
#define PACKET_REPLAY_PACKET_CONVERSATION_H

#include <queue>

#include "target_test_server.h"
#include "transport_packet.h"

namespace packet_replay {
    /**
     * Base class for a network conversation
     */
    class PacketConversation {
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
                    delete[] data_;
                }
            } Action;
            
            PacketConversation(const TransportPacket& packet, const TargetTestServer* target_test_server) {
                Layer3* layer3 = dynamic_cast<Layer3 *>(packet.getLayer(NETWORK));
                Layer4* layer4 = dynamic_cast<Layer4 *>(packet.getLayer(TRANSPORT));
    
                addr_family_ = layer3->getAddrFamily();
                addr_size_ = layer3->getAddrSize();
                sock_addr_size_ = layer3->getSockAddrSize();
    
                cap_src_addr_ = new uint8_t[addr_size_];
                cap_dest_addr_ = new uint8_t[addr_size_];
                test_dest_addr_ = new uint8_t[addr_size_];
    
                memcpy(cap_src_addr_, layer3->getSrcAddr(), addr_size_);
                memcpy(cap_dest_addr_, layer3->getDestAddr(), addr_size_);
    
                cap_src_port_ = layer4->getSrcPort();
                cap_dest_port_ = layer4->getDestPort();
    
                if (target_test_server) {
                    layer3->getAddrFromString(target_test_server->test_addr_, test_dest_addr_);
                    test_dest_port_ = htons(target_test_server->test_port_);
                } else {
                    memcpy(test_dest_addr_, cap_dest_addr_, addr_size_);
                    test_dest_port_ = cap_dest_port_;
                }
    
                test_sock_addr_ = new uint8_t[sock_addr_size_];
    
                layer3->getSockAddr(test_dest_addr_, test_dest_port_, test_sock_addr_);    
            }

            ~PacketConversation() {
                while (!action_queue_.empty()) {
                    delete action_queue_.front();
                    action_queue_.pop();
                }

                delete[] cap_src_addr_;
                delete[] cap_dest_addr_;
                delete[] test_dest_addr_;
                delete[] test_sock_addr_;
            }

            int getAddressFamily() {
                return addr_family_;
            }

            /**
             * Obtain the size of the socket address structure used for this connection
             */
            int getSockAddrSize() {
                return sock_addr_size_;
            }

            /**
             * Obtain a pointer to socket address structure of the test server
             */
            void* getTestSockAddr() {
                return test_sock_addr_;
            }

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
            std::queue<Action *> action_queue_;            
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

    };
}

#endif