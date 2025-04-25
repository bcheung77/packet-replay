#ifndef PACKET_REPLAY_CONFIGURED_CONVERSATION_H
#define PACKET_REPLAY_CONFIGURED_CONVERSATION_H

#include <string>

namespace packet_replay
{
    /**
     * Represents a target test server to use instead of the one in the capture.
     */
    class TargetTestServer {
        public:
            std::string test_addr_;
            int test_port_;

            TargetTestServer(const std::string& test_addr, int test_port) 
                : test_addr_(test_addr), test_port_(test_port) {
            }
    };

} // namespace packet_replay

#endif