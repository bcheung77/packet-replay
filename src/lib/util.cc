#include "util.h"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace packet_replay {
    std::vector<std::string> tokenize(const std::string& str, char delimiter) {
        std::vector<std::string> tokens;        
        std::stringstream str_stream(str);
        std::string token;

        while (std::getline(str_stream, token, ':')) {
            tokens.push_back(token);
        }

        return tokens;
    }

    std::string bytes_to_hex_string(const uint8_t *data, int size) {
        std::stringstream ss;
      
        ss << std::hex << std::setfill('0');
      
        for (int i = 0; i < size; i++) {
          ss << std::hex << std::setw(2) << static_cast<int>(data[i]);
        }
      
        return ss.str();
    }

    void trimLeft(std::string &s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        }));
    }
    
    void trimRight(std::string &s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), s.end());
    }

    void trim(std::string &s) {
        trimLeft(s);
        trimRight(s);
    }

    void toLower(std::string &s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return std::tolower(c); });
    }
} // namespace packet_replay