#include <string.h>
#include <unistd.h>

#include <iostream>

#include "tcp_conversation.h"

namespace packet_replay {
    void TcpConversation::processCapturePacket(const TransportPacket& packet) {
        TcpLayer* tcp_layer = dynamic_cast<TcpLayer *>(packet.getLayer(TRANSPORT));

        if (tcp_layer == nullptr) {
            // not tcp packet
            return;
        }

        // std::cout << "processing packet - data size " << tcp_layer->getDataSize() << std::endl;


        if (tcp_layer->hasRst()) {
            if (socket_ != -1) {
                close(socket_);
            }

            cap_tcp_state_ = CLOSED;
            return;
        }
        
        switch (cap_tcp_state_)
        {
            case CLOSED:
                closeProcessCapturePacket(packet);
                break;
            
            case SYN_SENT:
                synSentProcessCapturePacket(packet);
                break;

            case SYN_RECEIVED:
                synRecvProcessCapturePacket(packet);
            break;

            case ESTABLISHED:
                estProcessCapturePacket(packet);
            break;
        }
    }

    void TcpConversation::closeProcessCapturePacket(const TransportPacket& packet) {
        Layer3* layer3 = dynamic_cast<Layer3 *>(packet.getLayer(NETWORK));
        TcpLayer* tcp_layer = dynamic_cast<TcpLayer *>(packet.getLayer(TRANSPORT));

        if (memcmp(layer3->getSrcAddr(), cap_src_addr_, addr_size_) == 0 && tcp_layer->getFlags() == TH_SYN) {
            if (socket_ != -1) {
                // connection already exists ... 
                close(socket_);
                socket_ = -1;
             }

             cap_tcp_state_ = SYN_SENT;
        } else if (tcp_layer->getDataSize()) {
            // unexpected packet 
        } else {
            // assume close handshake
        }
    }

    void TcpConversation::synSentProcessCapturePacket(const TransportPacket& packet) {
        Layer3* layer3 = dynamic_cast<Layer3 *>(packet.getLayer(NETWORK));
        TcpLayer* tcp_layer = dynamic_cast<TcpLayer *>(packet.getLayer(TRANSPORT));

        if (memcmp(layer3->getSrcAddr(), cap_dest_addr_, addr_size_) == 0 && tcp_layer->hasAck() && tcp_layer->hasSyn()) {
            cap_tcp_state_ = SYN_RECEIVED;
        } else {
            // unexpected packet
        }
    }

    void TcpConversation::synRecvProcessCapturePacket(const TransportPacket& packet) {
        Layer3* layer3 = dynamic_cast<Layer3 *>(packet.getLayer(NETWORK));
        TcpLayer* tcp_layer = dynamic_cast<TcpLayer *>(packet.getLayer(TRANSPORT));

        if (memcmp(layer3->getSrcAddr(), cap_src_addr_, addr_size_) == 0 && tcp_layer->hasAck()) {
            cap_tcp_state_ = ESTABLISHED;

             Action* connect_action = new Action(CONNECT);
    
             action_queue_.push(connect_action);
        } else {
            // unexpected packet
        }
    }

    void TcpConversation::estProcessCapturePacket(const TransportPacket& packet) {
        Layer3* layer3 = dynamic_cast<Layer3 *>(packet.getLayer(NETWORK));
        TcpLayer* tcp_layer = dynamic_cast<TcpLayer *>(packet.getLayer(TRANSPORT));

        int data_size = tcp_layer->getDataSize();
        if (data_size) {
            Action* action;
            if (memcmp(layer3->getSrcAddr(), cap_src_addr_, addr_size_) == 0 && tcp_layer->getSrcPort() == cap_src_port_) {
                action = new Action(SEND);
            } else {
                action = new Action(RECV);
            }

            action->data_ = new uint8_t[data_size];
            memcpy(action->data_, tcp_layer->getData(), data_size);
            action->data_size_ = data_size;

            action_queue_.push(action);
        }

        if (tcp_layer->hasFin()) {
            cap_tcp_state_ = CLOSED;

            action_queue_.push(new Action(CLOSE));
        }
    }

}