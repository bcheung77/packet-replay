#include <string.h>

#include "packet_validator.h"
#include "python_api.h"

namespace packet_replay
{
    bool PacketValidator::validate(uint8_t* expected, int expected_len, uint8_t* actual, int actual_len) {
        if (expected_len != actual_len || memcmp(expected, actual, expected_len) != 0) {
            return false;
        }

        return true;
    }

    PythonPacketValidator::PythonPacketValidator(const std::string& python_file, const std::string& python_func) {
        call_ = PythonApi::getInstance()->createValidateCall(python_file, python_func);
    }

    PythonPacketValidator::~PythonPacketValidator() {
        delete call_;
    }

    bool PythonPacketValidator::validate(uint8_t* expected, int expected_len, uint8_t* actual, int actual_len) {
        return call_->validate(expected, expected_len, actual, actual_len);
    }
    
} // namespace packet_replay

