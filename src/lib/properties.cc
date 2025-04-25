#include "properties.h"

#include <ostream>
#include <string>
#include "util.h"

namespace packet_replay {
    bool Properties::put(std::string& line) {
        auto index = line.find(delimiter_) ;
        if (index == std::string::npos) {
            return false;
        }

        std::string key = line.substr(0, index);
        trim(key);
    
        std::string value = line.substr(index + delimiter_.length());
        trim(value);
    
        data_[key] = value;
        return true;
    }

    void Properties::write(std::ostream& output) {
        for (auto entry : data_) {
            output << entry.first << delimiter_ << " " << entry.second << std::endl;
        }

    }
} // namespace PACKET_REPLAY
