#ifndef PACKET_REPLAY_HTTP_RESPONSE_PROCESSOR_H
#define PACKET_REPLAY_HTTP_RESPONSE_PROCESSOR_H

#include <map>
#include <string>
#include <stdint.h>

namespace packet_replay {
    /**
     * Class to interpret a HTTP response.
     */
    class HttpResponseProcessor
    {
        private:
            std::string header_str_;
            std::map<std::string, std::string> headers_;
            uint8_t* payload_;
            int payload_size_;
            int payload_read_;
            int status_code_;

            void parseHeader(const std::string& header_str);
        public:
            HttpResponseProcessor() {
                reset();
            }

            ~HttpResponseProcessor() {
                delete payload_;
            }

            /**
             * Consume the response data.
             */
            bool processData(uint8_t* data, int data_size);

            /**
             * Reset this class to consume a new HTTP response
             */
            void reset() {
                payload_ = nullptr;
                payload_size_ = 0;
                payload_read_ = 0;
                status_code_ = -1;
                header_str_.resize(0);
            }

            /**
             * Whether all data from the response has been consumed.
             */
            bool complete() const {
                return status_code_ != -1 && payload_size_ == payload_read_;
            }

            int compare(const HttpResponseProcessor& other) const;
    };
}

#endif