#ifndef PACKET_REPLAY_NETWORK_LAYERS_H
#define PACKET_REPLAY_NETWORK_LAYERS_H

#include <stdexcept>
#include <string>

#include <stdint.h>

#include <string.h>

#include <arpa/inet.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>

namespace packet_replay {
    typedef enum {
        PROTO_ETHERNET,
        PROTO_IP,
        PROTO_IPv6,
        PROTO_TCP,
        PROTO_UDP
    } Protocol;

    typedef enum {
        PHYSICAL = 1,
        DATA_LINK,
        NETWORK,
        TRANSPORT,
        SESSION,
        PRESENTATION,
        APPLICATION
    } LayerNumber;

    /**
     * Base class for layers in the OSI model
     */
    class Layer {
        public:
            virtual Protocol getProtocol() = 0;
            virtual LayerNumber getLayerNumber() = 0;

            /**
             * Get the payload of the packet
             */
            virtual const uint8_t* getData() = 0;

            /**
             * Get the payload size of the packet
             */
            virtual int getDataSize() = 0;
        
        protected:
            const uint8_t* packet_;
            int packet_size_;

            Layer(const uint8_t* packet, int packet_size) : packet_(packet), packet_size_(packet_size) {
            }
    };

    class EthernetLayer : public Layer { 
        private:
            const struct ether_header* header_;

        public:
            EthernetLayer() = delete;
            EthernetLayer(const EthernetLayer&) = delete;
            EthernetLayer& operator=(const EthernetLayer&) = delete;

            EthernetLayer(const uint8_t* packet, int packet_size) : Layer(packet, packet_size) {
                header_ = reinterpret_cast<const struct ether_header*>(packet);
            }

            Protocol getProtocol() override {
                return PROTO_ETHERNET;
            }

            LayerNumber getLayerNumber() override {
                return DATA_LINK;
            }

            const uint8_t* getData() override {
                return packet_ + sizeof(struct ether_header);
            }

            uint16_t getEtherType() {
                return ntohs(header_->ether_type);
            }

            int getDataSize() override {
                return packet_size_ -  sizeof(struct ether_header);
            }
    };

    /**
     * Common base class for Layer 3 (Network) protocols
     */
    class Layer3 : public Layer {
        public:
            Layer3(const uint8_t* packet, int packet_size) : Layer(packet, packet_size) {
            }

            LayerNumber getLayerNumber() override {
                return NETWORK;
            }

            virtual const void* getSrcAddr() const {
                throw std::runtime_error("unsupported operation");
            }

            virtual const void* getDestAddr() const {
                throw std::runtime_error("unsupported operation");
            }

            /**
             * Get the source address as a string
             */
            virtual std::string getSrcAddrStr() const {
                throw std::runtime_error("unsupported operation");
            }

            virtual int getNextProtocol() const {
                throw std::runtime_error("unsupported operation");
            }
            
            virtual int getAddrFamily() const = 0;

            virtual int getAddrSize() const = 0;

            /**
             * Get the size of the socket address structure used for this layer
             */
            virtual int getSockAddrSize() const = 0;

            /**
             * Convert a string address to network address
             * 
             * @param addr_str the address in string format
             * @param buf the buffer to put network address in.  Must have at least getAddrSize() bytes allocated.
             */
            virtual bool getAddrFromString(const std::string& addr_str, void* buf) const = 0;

            /**
             * Convert the given address and port into a socket address
             * 
             * @param addr 
             * @param port
             * @param addr_buf the buffer to put the socket address in.  Must have at least getSockAddrSize() bytes allocated.
             */
            virtual void getSockAddr(const uint8_t* addr, int port, uint8_t* addr_buf) const = 0;
    };

    class IpV6LayerSpec : public Layer3 {
        public: 
            IpV6LayerSpec(const uint8_t* packet, int packet_size) : Layer3(packet, packet_size) {
            }

            IpV6LayerSpec() : Layer3(nullptr, 0) {
            }

            const uint8_t* getData() override {
                throw std::runtime_error("unsupported operation");
            }

            int getDataSize() override {
                throw std::runtime_error("unsupported operation");
            }

            Protocol getProtocol() override {
                return PROTO_IPv6;
            }

            int getAddrFamily() const override {
                return AF_INET6;
            }

            int getAddrSize() const override {
                return sizeof(reinterpret_cast<const struct ip6_hdr*>(0)->ip6_src);
            }

            int getSockAddrSize() const override {
                return sizeof(struct sockaddr_in6);
            }

            bool getAddrFromString(const std::string& addr_str, void* buf) const override {
                return inet_pton(AF_INET6, addr_str.c_str(), buf);
            }

            void getSockAddr(const uint8_t* addr, int port, uint8_t* addr_buf) const override {
                struct sockaddr_in6* output_addr = reinterpret_cast<struct sockaddr_in6*>(addr_buf);

                output_addr->sin6_family = getAddrFamily();
                memcpy (&output_addr->sin6_addr.s6_addr, addr, getAddrSize());
                output_addr->sin6_port = port;
            }
    };

    class IpV6Layer : public IpV6LayerSpec {
        private:
            const struct ip6_hdr* header_;

        public:
            IpV6Layer() = delete;
            IpV6Layer(const IpV6Layer&) = delete;
            IpV6Layer& operator=(const IpV6Layer&) = delete;

            IpV6Layer(const uint8_t* packet, int packet_size) : IpV6LayerSpec(packet, packet_size) {
                header_ = reinterpret_cast<const struct ip6_hdr*>(packet_);
            }

            const uint8_t* getData() override {
                return packet_ + (sizeof(*header_));
            }

            int getDataSize() override {
                return ntohs(header_->ip6_ctlun.ip6_un1.ip6_un1_plen);
            }

            int getNextProtocol() const override {
                return header_->ip6_ctlun.ip6_un1.ip6_un1_nxt;
            }

            const void* getSrcAddr() const override {
                return &header_->ip6_src;
            }

            const void* getDestAddr() const override {
                return &header_->ip6_dst;
            }

            std::string getSrcAddrStr() const override {
                char ip_str[INET6_ADDRSTRLEN];
                inet_ntop(AF_INET6, &header_->ip6_src, ip_str, INET6_ADDRSTRLEN);

                return std::string(ip_str);
            }
    };

    class IpLayerSpec : public Layer3 {
        public: 
            IpLayerSpec(const uint8_t* packet, int packet_size) : Layer3(packet, packet_size) {
            }

            IpLayerSpec() : Layer3(nullptr, 0) {
            }

            const uint8_t* getData() override {
                throw std::runtime_error("unsupported operation");
            }

            int getDataSize() override {
                throw std::runtime_error("unsupported operation");
            }

            Protocol getProtocol() override {
                return PROTO_IP;
            }

            int getAddrFamily() const override {
                return AF_INET;
            }

            int getAddrSize() const override {
                return sizeof(reinterpret_cast<const struct ip*>(0)->ip_src);
            }

            int getSockAddrSize() const override {
                return sizeof(struct sockaddr_in);
            }

            bool getAddrFromString(const std::string& addr_str, void* buf) const override {
                return inet_pton(AF_INET, addr_str.c_str(), buf);
            }

            void getSockAddr(const uint8_t* addr, int port, uint8_t* addr_buf) const override {
                struct sockaddr_in* output_addr = reinterpret_cast<struct sockaddr_in*>(addr_buf);

                output_addr->sin_family = getAddrFamily();
                memcpy (&output_addr->sin_addr.s_addr, addr, getAddrSize());
                output_addr->sin_port = port;
            }
    };

    class IpLayer : public IpLayerSpec {
        private:
            const struct ip* header_;

        public:
            IpLayer() = delete;
            IpLayer(const IpLayer&) = delete;
            IpLayer& operator=(const IpLayer&) = delete;

            IpLayer(const uint8_t* packet, int packet_size) : IpLayerSpec(packet, packet_size) {
                header_= reinterpret_cast<const struct ip*>(packet_);
            }

            const uint8_t* getData() override {
                return packet_ + (header_->ip_hl * 4);
            }

            int getDataSize() override {
                return ntohs(header_->ip_len) -  (header_->ip_hl * 4);
            }

            int getNextProtocol() const override {
                return header_->ip_p;
            }

            const void* getSrcAddr() const override {
                return &header_->ip_src;
            }

            const void* getDestAddr() const override {
                return &header_->ip_dst;
            }

            std::string getSrcAddrStr() const override {
                char ip_str[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &header_->ip_src, ip_str, INET_ADDRSTRLEN);

                return std::string(ip_str);
            }
    };

    class Layer4 : public Layer {
        public:
            Layer4(const uint8_t* packet, int packet_size) : Layer(packet, packet_size) {
            }

            virtual uint16_t getSrcPort() = 0;
            virtual uint16_t getDestPort() = 0;
    };

    class TcpLayer : public Layer4 {
        private:
            const struct tcphdr* header_;

        public:
            TcpLayer() = delete;
            TcpLayer(const TcpLayer&) = delete;
            TcpLayer& operator=(const TcpLayer&) = delete;

            TcpLayer(const uint8_t* packet, int packet_size) : Layer4(packet, packet_size) {
                header_= reinterpret_cast<const struct tcphdr*>(packet);
            }

            Protocol getProtocol() override {
                return PROTO_TCP;
            }

            LayerNumber getLayerNumber() override {
                return TRANSPORT;
            }

            const uint8_t* getData() override {
                return packet_ + (header_->th_off * 4);
            }

            int getDataSize() override {
                return packet_size_ - (header_->th_off * 4);
            }

            bool hasSyn() const {
                return header_->th_flags & TH_SYN;
            }

            bool hasAck() const {
                return header_->th_flags & TH_ACK;
            }

            bool hasRst() const {
                return header_->th_flags & TH_RST;
            }

            bool hasFin() const {
                return header_->th_flags & TH_FIN;
            }

            uint16_t getFlags() const {
                return header_->th_flags;
            }

            uint16_t getSrcPort() override {
                return header_->source;
            }

            uint16_t getDestPort() override {
                return header_->dest;
            }
    };
    
    class UdpLayer : public Layer4 {
        private:
            const struct udphdr* header_;

        public:
            UdpLayer() = delete;
            UdpLayer(const UdpLayer&) = delete;
            UdpLayer& operator=(const UdpLayer&) = delete;

            UdpLayer(const uint8_t* packet, int packet_size) : Layer4(packet, packet_size) {
                header_= reinterpret_cast<const struct udphdr*>(packet);
            }

            Protocol getProtocol() override {
                return PROTO_UDP;
            }

            LayerNumber getLayerNumber() override {
                return TRANSPORT;
            }

            const uint8_t* getData() override {
                return packet_ + sizeof(struct udphdr);
            }

            int getDataSize() override {
                return ntohs(header_->len) - sizeof(struct udphdr);
            }

            uint16_t getSrcPort() override {
                return header_->source;
            }

            uint16_t getDestPort() override {
                return header_->dest;
            }
    };
}

#endif