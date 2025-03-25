#ifndef PACKET_REPLAY_REST_CLIENT_H
#define PACKET_REPLAY_REST_CLIENT_H

#include "capture.h"
#include "tcp_conversation.h"

namespace packet_replay {
    /**
     * A HTTP client used to replay a conversation
     */
    class HttpReplayClient {
        private:
            TcpConversation* conversation_;
            int socket_;

            HttpReplayClient(const HttpReplayClient&) = delete;
            HttpReplayClient& operator=(const HttpReplayClient&) = delete;
            HttpReplayClient() = delete;

        public:
            HttpReplayClient(TcpConversation* conversation) : conversation_(conversation), socket_(-1) {
            }
            
            void replay();
    };
}

#endif