#ifndef PACKET_REPLAY_ACTION_H
#define PACKET_REPLAY_ACTION_H

#include <memory>
#include <vector>

namespace packet_replay
{
    /**
     * Describes an action that took place in the capture.
     */
    class Action {
        public:
            enum class Type {
                CONNECT,
                SEND,
                RECV,
                CLOSE
            };

            class SubToken {
                public:
                    SubToken(const std::string&& token, int begin_idx, int end_idx) : 
                        token_(token), begin_idx_(begin_idx), end_idx_(end_idx) {
                    }

                    const std::string& get_token() const {
                        return token_;
                    }

                    int get_begin() const {
                        return begin_idx_;
                    }

                    int get_end() const {
                        return end_idx_;
                    }

                protected:
                    SubToken() = default;
                    std::string token_;
                    int begin_idx_;
                    int end_idx_;
            };


            class OffsettedSubToken : public SubToken {
                public:
                    OffsettedSubToken(const SubToken* orig, int offset) {
                        token_ = orig->get_token();
                        begin_idx_ = orig->get_begin() + offset;
                        end_idx_ = orig->get_end() + offset;
                    }
            };

            const Type type_;

            Action(Type type) : type_(type) {
            }

            Action(Type type, std::vector<char>&& data) : type_(type), data_(data) {
            }
            
            Action(Type type, const char* data, int data_len) : type_(type) {
                data_.insert(data_.cend(), data, data + data_len);
            }

            Action() = default;

            void addSubToken(const std::string_view& token, int begin_idx, int end_idx) {
                subTokens_.push_back(make_unique<SubToken>(std::string(token), begin_idx, end_idx));
            }

            const std::vector<char>& data() const {
                return data_;
            }

        private:
            std::vector<char> data_;
            std::vector<std::unique_ptr<SubToken>> subTokens_;
    };
}


#endif