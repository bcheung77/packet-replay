#ifndef PACKET_REPLAY_NETWORK_LAYERS_H
#define PACKET_REPLAY_NETWORK_LAYERS_H

#include <stdint.h>

#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

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

            uint32_t getSrcIp() {
                return header_->ip_src.s_addr;
            }

            uint32_t getDestIp() {
                return header_->ip_dst.s_addr;
            }
    };

    class TcpLayer : public Layer {
        private:
            const struct tcphdr* header_;

        public:
            TcpLayer() = delete;
            TcpLayer(const TcpLayer&) = delete;
            TcpLayer& operator=(const TcpLayer&) = delete;

            TcpLayer(const uint8_t* packet, int packet_size) : Layer(packet, packet_size) {
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
    
}

#endif