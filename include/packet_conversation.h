#ifndef PACKET_REPLAY_PACKET_CONVERSATION_H
#define PACKET_REPLAY_PACKET_CONVERSATION_H

#include <queue>

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

            virtual int getAddressFamily() = 0;

            /**
             * Obtain the size of the socket address structure used for this connection
             */
            virtual int getSockAddrSize() = 0;

            /**
             * Obtain a pointer to socket address structure of the test server
             */
            virtual void* getTestSockAddr() = 0;

            /**
             * Record the specified packet into this conversation.
             */
            virtual void processCapturePacket(const TransportPacket& packet) = 0;

        protected:
            std::queue<Action *> action_queue_;            
    };
}

#endif