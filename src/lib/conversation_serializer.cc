#include <iterator>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "action.h"
#include "base64.h"
#include "conversation_serializer.h"
#include "packet_conversation.h"
#include "tcp_conversation.h"
#include "properties.h"
#include "target_test_server.h"
#include "util.h"
#include "udp_conversation.h"

namespace packet_replay {

    static const char ACTION_CONNECT[] = "CONNECT";
    static const char ACTION_SEND[] = "SEND";
    static const char ACTION_RECV[] = "RECV";
    static const char ACTION_CLOSE[] = "CLOSE";

    static const std::string TEST_ADDRESS_PROP = "TestAddress";
    static const std::string TEST_PORT_PROP = "TestPort";
    static const std::string ENCODING_PROP = "Encoding";
    static const std::string PROTOCOL_PROP = "Protocol";

    static const char BASE64_ENCODING[] = "BASE64";

    static bool is_comment(std::string& line) {
        for (int i = 0; i < line.length(); i++) {
            if (!isspace(line[i])) {
                if (line[i] == '#') {
                    return true;
                } else {
                    break;
                }
            }
        }

        return false;
    }

    static bool getline(std::istream& input, std::string& line) {
        while (std::getline(input, line)) {
            if (!is_comment(line)) {
                return true;
            }
        }

        return false;
    }

    static std::string action_type_to_string(Action::Type type) {
        switch (type) {
            case Action::Type::CONNECT:
                return "CONNECT";
            case Action::Type::SEND:
                return "SEND";
            case Action::Type::RECV:
                return "RECV";
            case Action::Type::CLOSE:
                return "CLOSE";
            default:
                throw std::runtime_error("invalid action type " + std::to_string(static_cast<int>(type)));
        }
    }

    static Action::Type string_to_action_type(std::string& type) {

        if (type == ACTION_CONNECT) {
            return Action::Type::CONNECT;  
        }

        if (type == ACTION_SEND) {
            return Action::Type::SEND;  
        }

        if (type == ACTION_RECV) {
            return Action::Type::RECV;  
        }

        if (type == ACTION_CLOSE) {
            return Action::Type::CLOSE;  
        }

        throw std::runtime_error("invalid action type " + type);
    }

    static void write_headers(std::ostream& output, const PacketConversation* conversation) {
        Properties props;

        props.put(PROTOCOL_PROP, conversation->getProtocol());

        char ip_str[INET6_ADDRSTRLEN];
        std::string port_str;

        switch (conversation->getAddressFamily()) {
            case AF_INET: {
                struct sockaddr_in* saddr = reinterpret_cast<struct sockaddr_in*>(conversation->getTestSockAddr());

                port_str = std::to_string(ntohs(saddr->sin_port));

                if (inet_ntop(AF_INET, &saddr->sin_addr, ip_str, INET_ADDRSTRLEN) == nullptr) {
                    throw std::runtime_error("Cannot translate address");
                }

                break;
            }

            case AF_INET6: {
                struct sockaddr_in6* saddr = reinterpret_cast<struct sockaddr_in6 *>(conversation->getTestSockAddr());

                port_str = std::to_string(ntohs(saddr->sin6_port));

                if (inet_ntop(AF_INET6, &saddr->sin6_addr.s6_addr, ip_str, INET6_ADDRSTRLEN) == nullptr) {
                    throw std::runtime_error("Cannot translate address");
                }

                break;
            }

            default:
                throw std::runtime_error("Unsupported address family");        
        }

        props.put(TEST_PORT_PROP, port_str);

        props.write(output);
    }

    std::vector<char> ConversationSerializer::read_action_data(std::istream& input) {
        std::vector<char> data;
                    
        constexpr auto buf_size = BUFSIZ;
        char buf[buf_size + 1];
        char* read_ptr = buf;
        auto size = buf_size;

        char delim = data_end_tag_[data_end_tag_.length() - 1];
        
        auto end_tag_len = data_end_tag_.length();

        do {
            // use peek to work around the fact that "get" will return 0 fail the stream if the next char is the delimiter
            auto check = input.peek() == delim;
            auto nread = 0;

            if (!check) {
                input.get(read_ptr, size, delim);
                nread = input.gcount();
                if (nread <= 0) {
                    throw std::runtime_error("Failed to process file");
                }

                if (nread != size - 1) {
                    check = true;
                }
                
                data.insert(data.cend(), buf, read_ptr + nread - end_tag_len);
                memcpy(buf, read_ptr + nread - end_tag_len, end_tag_len);
            }

            if (check) {
                buf[end_tag_len] = input.get();
                buf[end_tag_len + 1] = '\0';

                if (data_end_tag_ == buf + 1) {
                    data.push_back(buf[0]);
                    break;
                } else {
                    read_ptr = buf + end_tag_len + 1;
                    size = buf_size - (end_tag_len + 1);
                }
            } else {
                read_ptr = buf + end_tag_len;
                size = buf_size - end_tag_len;
            }

            if (input.eof()) {
                break;
            }
        } while (true);

        return data;
    }

    void ConversationSerializer::read_actions(std::istream& input, PacketConversation& conversation) {
        std::string line;
        Action::Type type;
        Properties prop;
        std::vector<char> data;

        while (!input.eof()) {
            bool found_action = false;
            data.clear();
            prop.clear();

            while (getline(input, line)) {
                if (!trim(line).empty()) {
                    type = string_to_action_type(line);
                    found_action = true;
                    break;
                }
            }

            while (getline(input, line)) {
                trim(line);
                if (line.empty()) {
                    continue;
                }

                if (line == data_start_tag_) {
                    data = read_action_data(input);
                    break;
                } else {
                    prop.put(line);
                }
            }

            if (found_action) {
                if (prop.contains(ENCODING_PROP) && prop.get(ENCODING_PROP) == BASE64_ENCODING) {
                    auto decoded = base64::from_base64(std::string_view(data.data(), data.size()));

                    data.clear();
                    data.insert(data.cend(), decoded.c_str(), decoded.c_str() + decoded.length());
                }

                Action* action = create_action(type, std::move(data));

                conversation.actionPush(action);
            }
        }
    }

    void ConversationSerializer::write_action(std::ostream& output, const Action* action) {
        output << action_type_to_string(action->type_) << std::endl;

        Properties prop;

        auto is_binary = false;
        auto data = action->data();
        auto data_size = data.size();

        for (int i = data_size - 1; i >= 0 && i > data_size - 50; i--) {
            if (!isprint(data[i]) && !isspace(data[i])) {
                is_binary = true;
                break;
            }
        }

        if (is_binary) {
            prop.put(ENCODING_PROP, BASE64_ENCODING);
            prop.write(output);
            output << data_start_tag_ << std::endl;

            auto data_view = std::string_view(data.data(), data_size);
            auto encoded_str = base64::to_base64(data_view);

            output << encoded_str;
        } else {
            output << data_start_tag_ << std::endl;
            output.write(data.data(), data_size);
        }
        output << data_end_tag_ << std::endl;
    }

    void ConversationSerializer::write(std::ostream& output, const PacketConversation* conversation) {  
        write_headers(output, conversation);
        for (auto action : conversation->getActionQueue()) {
            const char separator[] = "##############################";
            output << std::endl;
            output.write(separator, sizeof(separator) - 1);
            output << std::endl << std::endl;
            write_action(output, action);
        }
    }

    std::unique_ptr<PacketConversation> ConversationSerializer::read(std::istream& input) {

        std::unique_ptr<PacketConversation> ptr(nullptr);
        Properties headers;

        std::string line;
        while (getline(input, line)) {
            if (trimLeft(line).empty()) {
                break;
            }

            headers.put(line);
        }


        std::unique_ptr<Layer3> network_layer(nullptr);
        struct addrinfo hint;
        struct addrinfo* res = nullptr;
    
        memset(&hint, '\0', sizeof hint);
    
        hint.ai_family = PF_UNSPEC;
        hint.ai_flags = AI_NUMERICHOST;
    
        auto ret = getaddrinfo(headers.get(TEST_ADDRESS_PROP).c_str(), NULL, &hint, &res);
        if (ret) {
            throw std::runtime_error("Unsupported address: " + headers.get(TEST_ADDRESS_PROP) + " " + gai_strerror(ret));
        }

        if (res->ai_family == AF_INET) {
            network_layer.reset(new IpLayerSpec());
        } else {
            throw std::runtime_error("Unsupported address family: " + headers.get(TEST_ADDRESS_PROP));
        }

        freeaddrinfo(res);

        auto protocol = headers.get(PROTOCOL_PROP);

        if (protocol == TcpConversation::PROT_NAME) {
            TargetTestServer target_server(headers.get(TEST_ADDRESS_PROP), headers.getAsInt(TEST_PORT_PROP));
            ptr.reset(new TcpConversation(*network_layer, target_server));
        } else if (protocol == UdpConversation::PROT_NAME) {
            throw std::runtime_error("UDP script not supported");
        }

        read_actions(input, *ptr);

        return ptr;
    }

    Action* ConversationSerializer::create_action(Action::Type type, std::vector<char>&& data) {
        Action* action = new Action(type, std::move(data));

        std::string_view view(data.data(), data.size());

        auto start_idx = 0;
        auto prefix_len = subPrefix_.length();
        auto suffix_len = subSuffix_.length();

        while (start_idx != std::string_view::npos) {
            start_idx = view.find(subPrefix_, start_idx);

            if (start_idx != std::string_view::npos)  {
                auto end_idx = view.find(subSuffix_, start_idx + prefix_len);

                if (end_idx != std::string_view::npos) {
                    action->addSubToken(view.substr(start_idx + prefix_len, end_idx), start_idx, end_idx + suffix_len);
                    start_idx = end_idx + suffix_len;
                } else {
                    break;
                }
            } else {
                break;
            }
        }

        return action;
    }

} // packet-replay