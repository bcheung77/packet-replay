#include <string.h>

#include "packet_validator.h"
#include "python_api.h"

namespace packet_replay
{
    bool PacketValidator::validate(const uint8_t* expected, int expected_len, uint8_t* actual, int actual_len) {
        if (expected_len != actual_len || memcmp(expected, actual, expected_len) != 0) {
            return false;
        }

        return true;
    }
    
    bool PacketValidator::validate(const uint8_t* expected, int expected_len, uint8_t* actual, int actual_len, 
        std::vector<Action::SubToken *>& sub_tokens, Properties& context) {

        int actual_idx = 0;
        int start_idx = 0;
        for (auto sub_token : sub_tokens) {
            auto len = sub_token->get_begin() - start_idx;

            if (len > 0 && memcmp(expected + start_idx, actual + start_idx, len) != 0) {
                return false;
            }

            if (context.contains(sub_token->get_token())) {
                auto value = context.get(sub_token->get_token());

                if (memcmp(value.c_str(), actual + start_idx, value.size()) != 0) {
                    return false;
                } else {
                    actual_idx += value.size();
                }
            } else if (memcmp(expected + start_idx, actual + actual_idx, sub_token->get_end() - sub_token->get_begin()) != 0) {
                return false;
            } else {
                actual_idx += sub_token->get_end() - sub_token->get_begin();
            }

            start_idx = sub_token->get_end();
        }

        auto remaining_actual = actual_len - actual_idx;
        auto remaining_expected = expected_len - start_idx;

        if (remaining_actual != remaining_expected ||
            memcmp(actual + actual_idx, expected + expected_len, remaining_actual) != 0) {
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

    bool PythonPacketValidator::validate(const uint8_t* expected, int expected_len, uint8_t* actual, int actual_len) {
        return call_->validate(expected, expected_len, actual, actual_len);
    }
    
} // namespace packet_replay

