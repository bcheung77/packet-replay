#include <iostream>
#include <map>
#include <set>
#include <stdexcept>

#include <net/ethernet.h>
#include <netinet/ip.h>
#include <pcap/pcap.h>

#include "capture.h"
#include "network_layers.h"
#include "packet_conversation.h"
#include "transport_packet.h"

namespace packet_replay {

    void Capture::packetHandler(const struct pcap_pkthdr *h, const u_char *bytes) {

        static auto packet_num = 1;
    
        if (h->caplen != h->len) {
            throw std::runtime_error("packet not fully captured.  increase snap length on capture");
        }

        // std::cout << packet_num++ << std::endl;
        
        TransportPacket packet;
        Layer3* network_layer;

        switch (datalink_type_) {
            case DLT_NULL:
                if (bytes[0] == AF_INET || bytes[3] == AF_INET) {
                    network_layer = new IpLayer(bytes + 4, h->caplen - 4);
                } else {
                    return;
                }
                break;
    
            case DLT_EN10MB: {
                    EthernetLayer* eth_layer = new EthernetLayer(bytes, h->caplen);
                    packet.addLayer(eth_layer);

                    switch (eth_layer->getEtherType()) {
                        case ETHERTYPE_IP:
                            network_layer = new IpLayer(eth_layer->getData(), eth_layer->getDataSize());
                            break;

                        case ETHERTYPE_IPV6:
                            network_layer = new IpV6Layer(eth_layer->getData(), eth_layer->getDataSize());
                            break; 
                            
                        default:
                            return;
                    }
                }
                break;
    
            default:
                return;
        }
    
        // char source_ip[INET_ADDRSTRLEN];
        // char dest_ip[INET_ADDRSTRLEN];
        // inet_ntop(AF_INET, &(ip_header->ip_src), source_ip, INET_ADDRSTRLEN);
        // inet_ntop(AF_INET, &(ip_header->ip_dst), dest_ip, INET_ADDRSTRLEN);
    
        // std::cout << packet_num++ << " src: " << source_ip << " dest: " << dest_ip << std::endl;

        packet.addLayer(network_layer);

        packet_replay::PacketConversation* conversation = nullptr;
        switch (network_layer->getNextProtocol()) {
            case IPPROTO_TCP: {
                TcpLayer* tcp_layer = new TcpLayer(network_layer->getData(), network_layer->getDataSize());
                packet.addLayer(tcp_layer);
                break;
            }
    
            case IPPROTO_UDP: {
                UdpLayer* udp_layer = new UdpLayer(network_layer->getData(), network_layer->getDataSize());
                packet.addLayer(udp_layer);
                break;
            }

            default:
                return;
        }

        conversation = conversation_store_.getConversation(packet);

        if (conversation) {
            // std::cout << "processing packet ..." << std::endl;
            conversation->processCapturePacket(packet);
        }

    }

    static void packet_handler_func(u_char *this_ptr, const struct pcap_pkthdr *h, const u_char *bytes) {
        Capture* thisObject = reinterpret_cast<Capture *>(this_ptr);

        thisObject->packetHandler(h, bytes);
    }

    void Capture::load(const char* capture_file) {
        char errbuf[PCAP_ERRBUF_SIZE];
        pcap_t* pcap = pcap_open_offline(capture_file, errbuf);

        if (!pcap) {
            throw std::runtime_error("failed to open file: " + std::string(errbuf));
        }

        datalink_type_ = pcap_datalink(pcap);

        int rc = pcap_loop(pcap, 0, packet_handler_func, reinterpret_cast<u_char *>(this));
        if (rc != 0) {
            if (rc == PCAP_ERROR) {
                throw std::runtime_error("packet read failed " + std::string(pcap_geterr(pcap)));
            } else {
                throw std::runtime_error("packet read failed " + std::to_string(rc));
            }
        }

        if (pcap) {
            pcap_close(pcap);
        }   
    }

}
