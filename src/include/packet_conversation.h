#ifndef PACKET_REPLAY_PACKET_CONVERSATION_H
#define PACKET_REPLAY_PACKET_CONVERSATION_H

#include <deque>
#include <memory>
#include <vector>

#include "action.h"
#include "target_test_server.h"
#include "transport_packet.h"

namespace packet_replay {
    /**
     * Base class for a network conversation
     */
    class PacketConversation {
        public:
            PacketConversation(const Layer3& layer3, const TargetTestServer& target_test_server) {
                addr_family_ = layer3.getAddrFamily();
                addr_size_ = layer3.getAddrSize();
                sock_addr_size_ = layer3.getSockAddrSize();

                test_dest_addr_ = std::make_unique<uint8_t[]>(addr_size_);
                test_dest_port_ = htons(target_test_server.test_port_);
                layer3.getAddrFromString(target_test_server.test_addr_, test_dest_addr_.get());

                test_sock_addr_ = std::make_unique<uint8_t[]>(sock_addr_size_);
    
                layer3.getSockAddr(test_dest_addr_.get(), test_dest_port_, test_sock_addr_.get());    
            }

            PacketConversation(const TransportPacket& packet, const TargetTestServer* target_test_server) {
                Layer3* layer3 = dynamic_cast<Layer3 *>(packet.getLayer(NETWORK));
                Layer4* layer4 = dynamic_cast<Layer4 *>(packet.getLayer(TRANSPORT));
    
                addr_family_ = layer3->getAddrFamily();
                addr_size_ = layer3->getAddrSize();
                sock_addr_size_ = layer3->getSockAddrSize();
    
                cap_src_addr_ = std::make_unique<uint8_t[]>(addr_size_);
                cap_dest_addr_ = std::make_unique<uint8_t[]>(addr_size_);
                test_dest_addr_ = std::make_unique<uint8_t[]>(addr_size_);
    
                memcpy(cap_src_addr_.get(), layer3->getSrcAddr(), addr_size_);
                memcpy(cap_dest_addr_.get(), layer3->getDestAddr(), addr_size_);
    
                cap_src_port_ = layer4->getSrcPort();
                cap_dest_port_ = layer4->getDestPort();
    
                if (target_test_server) {
                    layer3->getAddrFromString(target_test_server->test_addr_, test_dest_addr_.get());
                    test_dest_port_ = htons(target_test_server->test_port_);
                } else {
                    memcpy(test_dest_addr_.get(), cap_dest_addr_.get(), addr_size_);
                    test_dest_port_ = cap_dest_port_;
                }
    
                test_sock_addr_ = std::make_unique<uint8_t[]>(sock_addr_size_);
    
                layer3->getSockAddr(test_dest_addr_.get(), test_dest_port_, test_sock_addr_.get());    
            }

            ~PacketConversation() {
                while (!action_queue_.empty()) {
                    delete action_queue_.front();
                    action_queue_.pop_front();
                }
            }

            int getAddressFamily() const {
                return addr_family_;
            }

            /**
             * Obtain the size of the socket address structure used for this connection
             */
            int getSockAddrSize() const {
                return sock_addr_size_;
            }

            /**
             * Obtain a pointer to socket address structure of the test server
             */
            void* getTestSockAddr() const {
                return test_sock_addr_.get();
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
                action_queue_.pop_front();
            }

            void actionPush(Action* action) {
                action_queue_.push_back(action);
            }

            const std::deque<Action *> getActionQueue() const {
                return action_queue_;
            }

            /**
             * Record the specified packet into this conversation.
             */
            virtual void processCapturePacket(const TransportPacket& packet) = 0;

            /**
             * The protocol used in the conversation
             */
            virtual const char* getProtocol() const = 0;

        protected:
            PacketConversation() = default;

            std::deque<Action *> action_queue_;            
            int addr_family_;
            int addr_size_;
            int sock_addr_size_;
            std::unique_ptr<uint8_t[]> test_sock_addr_;
            std::unique_ptr<uint8_t[]> cap_src_addr_;
            std::unique_ptr<uint8_t[]> cap_dest_addr_;
            std::unique_ptr<uint8_t[]> test_dest_addr_;

            uint16_t cap_dest_port_;
            uint16_t cap_src_port_;
            uint16_t test_dest_port_;
    };
}

#endif