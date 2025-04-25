#ifndef PACKET_REPLAY_PROPERTIE_H_
#define PACKET_REPLAY_PROPERTIE_H_

#include <ostream>
#include <string>
#include <unordered_map>

namespace packet_replay
{
    class Properties
    {
        private:
            std::unordered_map<std::string, std::string> data_;
            std::string delimiter_ = ":";
            
        public:
            Properties() = default;
            ~Properties() = default;

            void put(const std::string& key, const std::string& value) {
                data_[key] = value;
            }

            void put(const char* key, const std::string& value) {
                data_[key] = value;
            }

            void put(const char* key, const char* value) {
                data_[key] = value;
            }

            bool contains(const std::string& key) const {
                return data_.contains(key);
            }

            const std::string& get(const std::string& key) const {
                return data_.at(key);
            }

            int getAsInt(const std::string& key) const {
                return std::stoi(data_.at(key));
            }

            void clear() {
                data_.clear();
            }

            bool put(std::string& line);

            void write(std::ostream& output);
        };

} // namespace PACKET_REPLAY


#endif