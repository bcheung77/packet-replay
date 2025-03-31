#ifndef PACKET_REPLAY_REST_CLIENT_H
#define PACKET_REPLAY_REST_CLIENT_H

#include <unistd.h>

#include "capture.h"
#include "packet_validator.h"
#include "udp_conversation.h"

namespace packet_replay {
    /**
     * A HTTP client used to replay a conversation
     */
    class UdpReplayClient {
        private:
            UdpConversation* conversation_;
            int socket_;
            PacketValidator* validator_;

            UdpReplayClient(const UdpReplayClient&) = delete;
            UdpReplayClient& operator=(const UdpReplayClient&) = delete;
            UdpReplayClient() = delete;

        public:
            /**
             * @param conversation the conversation to reply
             * @param validator a pointer to the object that performs packet validation.  
             *                  Ownership is transferred to this object and will be freed by this object.
             */
            UdpReplayClient(UdpConversation* conversation, PacketValidator* validator);

            ~UdpReplayClient() {
                if (socket_ >= 0) {
                    close(socket_);
                }
                delete validator_;
            }
            
            void replay();
    };
}

#endif