#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include <sstream>
#include <stdexcept>
#include <string>

#include "http_response_processor.h"
#include "util.h"

namespace packet_replay {
    bool HttpResponseProcessor::processData(const std::vector<char>& data) {
        return processData(reinterpret_cast<const uint8_t*>(data.data()), data.size());
    }

    bool HttpResponseProcessor::processData(const uint8_t* data, int data_size) {
        if (status_code_ == -1) {
            auto orig_len = header_str_.length();
            header_str_.append(reinterpret_cast<const char *>(data), data_size);

            if (orig_len + data_size > 3) {
                int find_start = orig_len < 3 ? 0 : orig_len - 3;
                int pos = header_str_.find("\r\n\r\n", find_start);

                if (pos != std::string::npos) {
                    std::map<std::string, std::string>::iterator it;

                    parseHeader(header_str_.substr(0, pos));
                    if (headers_.contains("content-length")) {
                        data_processor_.reset(new HttpResponseProcessor::ContentLenProcessor(headers_));
                    } else if ((it = headers_.find("transfer-encoding")) != headers_.cend() && strcasecmp((*it).second.c_str(), "chunked") == 0) {
                        data_processor_.reset(new HttpResponseProcessor::ChunkedProcessor());
                    } else {
                        throw std::runtime_error("unsupported HTTP encoding");
                    }

                    if (header_str_.length() > pos + 4) {
                        int len = header_str_.length() - (pos + 4);

                        (*data_processor_).process(reinterpret_cast<const uint8_t *>(header_str_.substr(pos + 4).c_str()), len);
                        header_str_.resize(pos + 4);
                    }
                }
            } 
        } else {
            (*data_processor_).process(data, data_size);
        }

        if ((*data_processor_).isComplete()) {
            return true;
        } 

        return false;
    }

    int HttpResponseProcessor::compare(const HttpResponseProcessor& other) const {
        if (!complete() || !other.complete()) {
            throw std::runtime_error("internal failure: invalid state for response processor");
        }

        auto ret = status_code_ - other.status_code_;

        if (ret != 0) {
            return ret;
        }

        return (*data_processor_).compare(other.data_processor_.get());
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

    int HttpResponseProcessor::ContentLenProcessor::compare(const HttpResponseProcessor::DataProcessor* other) const {
        const ContentLenProcessor* typed_other = dynamic_cast<const ContentLenProcessor *>(other);

        if (typed_other == nullptr) {
            throw std::runtime_error("response used different transfer encoding");
        }

        if (typed_other->payload_size_ != payload_size_) {
            return payload_size_ - typed_other->payload_size_;
        }

        return memcmp(payload_, typed_other->payload_, payload_size_);
    }

    int HttpResponseProcessor::ChunkedProcessor::compare(const DataProcessor* other) const {
        auto typed_other = dynamic_cast<const ChunkedProcessor *>(other);

        if (typed_other == nullptr) {
            throw std::runtime_error("response used different transfer encoding");
        }

        int size_diff = data_.size() - typed_other->data_.size();

        if (size_diff != 0) {
            return size_diff;
        }

        return strncmp(data_.data(),  typed_other->data_.data(), data_.size());
    }

    bool HttpResponseProcessor::ChunkedProcessor::process(const uint8_t* data, int data_len) {
        auto nprocessed = 0;

        while (data_len > nprocessed) {
            if (chunk_size_ <  0) {
                auto copy_len = (data_len - nprocessed) < (TMPSIZ - tmp_buf_size_) ? data_len - nprocessed : (TMPSIZ - tmp_buf_size_);

                memcpy(tmp_buf_ + tmp_buf_size_, data + nprocessed, copy_len);
                tmp_buf_size_ += copy_len;
                tmp_buf_[tmp_buf_size_] = '\0';

                if (strstr(tmp_buf_, "\r\n") != nullptr) {
                    char* end_ptr;

                    chunk_size_ = strtoul(tmp_buf_, &end_ptr, 16);

                    if (end_ptr == tmp_buf_) {
                        throw std::runtime_error("invalid chunk size in " + std::string(tmp_buf_));
                    }

                    nprocessed += end_ptr - tmp_buf_ + 2;
                } else if (tmp_buf_size_ == TMPSIZ) {
                    throw std::runtime_error("failed to find chunk size");
                } else {
                    return false;
                }
            } else if (chunk_read_ == chunk_size_) {
                if (chunk_end_char_count_ == 0) {
                    if (data[nprocessed] == '\r') {
                        chunk_end_char_count_++;
                        nprocessed++;
                    } else {
                        throw std::runtime_error("invalid chunk terminator char " + data[nprocessed]);
                    }
                } else if (chunk_end_char_count_ == 1) {
                    if (data[nprocessed] == '\n') {
                        nprocessed++;

                        if (chunk_size_ == 0) {
                            complete_ = true;
                            return true;
                        }

                        // reset for new chunk
                        chunk_size_ = -1;
                        chunk_read_ = 0;
                        tmp_buf_size_ = 0;
                        chunk_end_char_count_ = 0;
                    } else {
                        throw std::runtime_error("invalid chunk terminator char " + data[nprocessed]);
                    }
                }
            } else {
                auto copy_len = (data_len - nprocessed) < (chunk_size_ - chunk_read_) ? data_len - nprocessed : chunk_size_ - chunk_read_;

                data_.insert(data_.cend(), (char *) data + nprocessed, (char *) data + nprocessed + copy_len);
                chunk_read_ += copy_len;
                nprocessed += copy_len;
            }
        }

        return false;
    }

}