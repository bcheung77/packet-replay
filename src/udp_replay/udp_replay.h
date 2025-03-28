#ifndef PACKET_REPLAY_REST_CLIENT_H
#define PACKET_REPLAY_REST_CLIENT_H

#include <unistd.h>

#include "capture.h"
#include "udp_conversation.h"

namespace packet_replay {
    /**
     * A HTTP client used to replay a conversation
     */
    class UdpReplayClient {
        private:
            UdpConversation* conversation_;
            int socket_;

            UdpReplayClient(const UdpReplayClient&) = delete;
            UdpReplayClient& operator=(const UdpReplayClient&) = delete;
            UdpReplayClient() = delete;

        public:
            UdpReplayClient(UdpConversation* conversation);

            ~UdpReplayClient() {
                if (socket_ >= 0) {
                    close(socket_);
                }
            }
            
            void replay();
    };
}

#endif