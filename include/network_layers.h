#ifndef PACKET_REPLAY_NETWORK_LAYERS_H
#define PACKET_REPLAY_NETWORK_LAYERS_H

#include <string>

#include <stdint.h>

#include <arpa/inet.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
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

            Protocol getProtocol() {
                return PROTO_ETHERNET;
            }

            LayerNumber getLayerNumber() {
                return DATA_LINK;
            }

            const uint8_t* getData() {
                return packet_ + sizeof(struct ether_header);
            }

            uint16_t getEtherType() {
                return ntohs(header_->ether_type);
            }

            int getDataSize() {
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

            LayerNumber getLayerNumber() {
                return NETWORK;
            }

            virtual const void* getSrcAddr() const = 0;

            virtual const void* getDestAddr() const = 0;

            /**
             * Get the source address as a string
             */
            virtual std::string getSrcAddrStr() const = 0;
            
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
            virtual void getSockAddr(const uint8_t* addr, int port, uint8_t* addr_buf) = 0;
    };

    class IpLayer : public Layer3 {
        private:
            const struct ip* header_;

        public:
            IpLayer() = delete;
            IpLayer(const IpLayer&) = delete;
            IpLayer& operator=(const IpLayer&) = delete;

            IpLayer(const uint8_t* packet, int packet_size) : Layer3(packet, packet_size) {
                header_= reinterpret_cast<const struct ip*>(packet_);
            }

            Protocol getProtocol() {
                return PROTO_IP;
            }

            const uint8_t* getData() {
                return packet_ + (header_->ip_hl * 4);
            }

            int getDataSize() {
                return ntohs(header_->ip_len) -  (header_->ip_hl * 4);
            }

            int getIpProtocol() {
                return header_->ip_p;
            }

            const void* getSrcAddr() const {
                return &header_->ip_src;
            }

            const void* getDestAddr() const {
                return &header_->ip_dst;
            }

            std::string getSrcAddrStr() const {
                char ip_str[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &header_->ip_src, ip_str, INET_ADDRSTRLEN);

                return std::string(ip_str);
            }

            int getAddrFamily() const {
                return AF_INET;
            }

            int getAddrSize() const {
                return sizeof(header_->ip_src);
            }

            int getSockAddrSize() const {
                return sizeof(struct sockaddr_in);
            }

            bool getAddrFromString(const std::string& addr_str, void* buf) const {
                return inet_pton(AF_INET, addr_str.c_str(), buf);
            }

            void getSockAddr(const uint8_t* addr, int port, uint8_t* addr_buf) {
                struct sockaddr_in* output_addr = reinterpret_cast<struct sockaddr_in*>(addr_buf);

                output_addr->sin_family = getAddrFamily();
                memcpy (&output_addr->sin_addr.s_addr, addr, getAddrSize());
                output_addr->sin_port = port;
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

            Protocol getProtocol() {
                return PROTO_TCP;
            }

            LayerNumber getLayerNumber() {
                return TRANSPORT;
            }

            const uint8_t* getData() {
                return packet_ + (header_->th_off * 4);
            }

            int getDataSize() {
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

            uint16_t getSrcPort() {
                return header_->source;
            }

            uint16_t getDestPort() {
                return header_->dest;
            }
    };
    
    class UdpLayer : public Layer {
        private:
            const struct udphdr* header_;

        public:
            UdpLayer() = delete;
            UdpLayer(const UdpLayer&) = delete;
            UdpLayer& operator=(const UdpLayer&) = delete;

            UdpLayer(const uint8_t* packet, int packet_size) : Layer(packet, packet_size) {
                header_= reinterpret_cast<const struct udphdr*>(packet);
            }

            Protocol getProtocol() {
                return PROTO_UDP;
            }

            LayerNumber getLayerNumber() {
                return TRANSPORT;
            }

            const uint8_t* getData() {
                return packet_ + sizeof(struct udphdr);
            }

            int getDataSize() {
                return ntohs(header_->len) - sizeof(struct udphdr);
            }
    };
}

#endif