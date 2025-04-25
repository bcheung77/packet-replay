#ifndef PACKET_REPLAY_UTIL_H
#define PACKET_REPLAY_UTIL_H

#include <string>
#include <vector>

#include <stdint.h>

namespace packet_replay {
    std::vector<std::string> tokenize(const std::string& str, char delimiter);

    std::string bytes_to_hex_string(const uint8_t* data, int size);

    std::string& trimLeft(std::string& s);
    
    std::string& trimRight(std::string& s);

    std::string& trim(std::string& s);

    std::string& toLower(std::string& s);
}

#endif