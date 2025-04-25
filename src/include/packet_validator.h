#ifndef PACKET_REPLAY_PACKET_VALIDATOR_H
#define PACKET_REPLAY_PACKET_VALIDATOR_H

#include <string>

#include <stdint.h>

#include "python_api.h"

namespace packet_replay
{
    /**
     * Class responsible for comparing packet from the capture to packets from the live session.  Default
     * implementation is a complete byte by byte comparison
     * 
     * @param expected the packet from the capture
     * @param expected_len the number of bytes in the capture packet
     * @param actual the packet from the live session
     * @param actual_len the number of bytes in the live session packet
     * 
     * @return if the 2 packets are equivalent, false otherwise
     */
    class PacketValidator {
        public:
            virtual bool validate(uint8_t* expected, int expected_len, uint8_t* actual, int actual_len);
            virtual ~PacketValidator() {}
    };
    
    /**
     * Packet validiation via Python callback
     */
    class PythonPacketValidator : public PacketValidator {
        public:
            /**
             * Constructor
             * 
             * @param python_file the Python file with the module containing the function to call
             * @param python_func the Python function to call to validate a packet.  The function must return bool and take in two
             *                    bytes object (expected, actual)
             */
            PythonPacketValidator(const std::string& python_file, const std::string& python_func);
            ~PythonPacketValidator();
            bool validate(uint8_t* expected, int expected_len, uint8_t* actual, int actual_len) override;

        private:
            ValidatePythonCall* call_;
    };

} // namespace packet_replay


#endif