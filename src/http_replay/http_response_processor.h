#ifndef PACKET_REPLAY_HTTP_RESPONSE_PROCESSOR_H
#define PACKET_REPLAY_HTTP_RESPONSE_PROCESSOR_H

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <stdint.h>

namespace packet_replay {
    /**
     * Class to interpret a HTTP response.
     */
    class HttpResponseProcessor
    {
        private:
            /**
             * Class to handle the payload portion of the HTTP response
             */
            class DataProcessor {
                public:
                    /** 
                     * Compare if the specified datga processor contains the same data 
                    */
                    virtual int compare(const DataProcessor* other) const = 0;

                    /**
                     * Process incoming data for the response
                     */
                    virtual bool process(const uint8_t* data, int data_len) = 0;

                    /**
                     * Flags whether this response payload has been fully processed.
                     */
                    virtual bool isComplete() const = 0;
            };

            /**
             * DataProcessor for responses that specify "content-length"
             */
            class ContentLenProcessor : public DataProcessor {
                uint8_t* payload_;
                int payload_size_;
                int payload_read_ = 0;
    
                public:
                    ContentLenProcessor(std::map<std::string, std::string> headers) {
                        payload_size_ = std::stoul(headers["content-length"]);
                        payload_ = new uint8_t[payload_size_];
                    }

                    ~ContentLenProcessor() {
                        delete[] payload_;
                    }

                    int compare(const DataProcessor* other) const override;

                    bool process(const uint8_t* data, int data_len) override {
                        memcpy(payload_ + payload_read_, data, data_len);
                        payload_read_ += data_len;

                        return isComplete();
                    }

                    bool isComplete() const override {
                        return payload_size_ == payload_read_;
                    }
            };

            /**
             * DataProcessor for responses that use "chunked" transfer-encoding
             */
            class ChunkedProcessor : public DataProcessor {
                constexpr static int TMPSIZ = 1024;
                int chunk_size_ = -1;
                int chunk_read_ = 0;
                int tmp_buf_size_ = 0;
                int chunk_end_char_count_ = 0;
                char tmp_buf_[TMPSIZ + 1];
                std::vector<char> data_;
                bool complete_ = false;

                public:
                    int compare(const DataProcessor* other) const override;
                    bool process(const uint8_t* data, int data_len) override;
                    bool isComplete() const override {
                        return complete_;
                    }
            };

            std::string header_str_;
            std::map<std::string, std::string> headers_;
            int status_code_;

            std::unique_ptr<DataProcessor> data_processor_;

            void parseHeader(const std::string& header_str);

        public:
            HttpResponseProcessor() {
                reset();
            }

            /**
             * Consume the response data.
             */
            bool processData(const std::vector<char>& data);
            bool processData(const uint8_t* data, int data_len);

            /**
             * Reset this class to consume a new HTTP response
             */
            void reset() {        
                status_code_ = -1;
                header_str_.resize(0);
                data_processor_.reset(nullptr);
            }

            /**
             * Whether all data from the response has been consumed.
             */
            bool complete() const {
                return status_code_ != -1 && (*data_processor_).isComplete();
            }

            int compare(const HttpResponseProcessor& other) const;
    };
}

#endif