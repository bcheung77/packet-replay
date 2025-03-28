
#include <string.h>

#include <sstream>
#include <stdexcept>
#include <string>

#include "http_response_processor.h"
#include "util.h"

namespace packet_replay {
    bool HttpResponseProcessor::processData(uint8_t* data, int data_size) {
        if (status_code_ == -1) {
            auto orig_len = header_str_.length();
            header_str_.append(reinterpret_cast<char *>(data), data_size);

            if (orig_len + data_size > 3) {
                int find_start = orig_len < 3 ? 0 : orig_len - 3;
                int pos = header_str_.find("\r\n\r\n", find_start);

                if (pos != std::string::npos) {
                    parseHeader(header_str_.substr(0, pos));
                    if (headers_.contains("content-length")) {
                        payload_size_ = std::stoul(headers_["content-length"]);
                        payload_ = new uint8_t[payload_size_];
    
                        if (header_str_.length() > pos + 4) {
                            int len = header_str_.length() - (pos + 4);
                            memcpy(payload_, header_str_.substr(pos + 4).c_str(), len);
                            payload_read_ += len;
                            header_str_.resize(pos + 4);
                        }
                    }
                }
            } 
        } else {
            memcpy(payload_ + payload_read_, data, data_size);
            payload_read_ += data_size;
        }

        if (payload_read_ == payload_size_) {
            return true;
        } 

        return false;
    }

    int HttpResponseProcessor::compare(const HttpResponseProcessor& other) const {
        if (!complete() || !other.complete()) {
            std::runtime_error("internal failure: invalid state for response processor");
        }

        auto ret = status_code_ - other.status_code_;

        if (ret != 0) {
            return ret;
        }

        ret = payload_size_ - other.payload_size_;

        if (ret != 0) {
            return ret;
        }

        ret = memcmp(payload_, other.payload_, payload_size_);

        return ret;
    }

    void HttpResponseProcessor::parseHeader(const std::string& header_str) {
        std::stringstream stream(header_str);

        std::string token;
        if (std::getline(stream, token, '\n')) {
            std::stringstream status(token);
            std::string status_token;

            if (std::getline(status, status_token, ' ') && std::getline(status, status_token, ' ')) {
                status_code_ = std::stoul(status_token);
            }

            while (std::getline(stream, token, '\n')) {
                std::stringstream header_stream(token);

                std::string key;
                std::string value;

                std::getline(header_stream, key, ':');
                std::getline(header_stream, value, '\r');

                trim(key);
                trim(value);

                toLower(key);

                headers_[key] = value;
            }
        }

    }

}