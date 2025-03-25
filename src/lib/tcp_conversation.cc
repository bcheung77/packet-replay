#include <string.h>
#include <unistd.h>

#include <iostream>

#include "tcp_conversation.h"

namespace packet_replay {
    void IpV4TcpConversation::processCapturePacket(const TransportPacket& packet) {
        TcpLayer* tcp_layer = dynamic_cast<TcpLayer *>(packet.getLayer(TRANSPORT));

        // std::cout << "processing packet - data size " << tcp_layer->getDataSize() << std::endl;


        if (tcp_layer->hasRst()) {
            if (socket_ != -1) {
                close(socket_);
            }

            capTcpState_ = CLOSED;
            return;
        }
        
        switch (capTcpState_)
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

    void IpV4TcpConversation::closeProcessCapturePacket(const TransportPacket& packet) {
        IpLayer* ip_layer = dynamic_cast<IpLayer *>(packet.getLayer(NETWORK));
        TcpLayer* tcp_layer = dynamic_cast<TcpLayer *>(packet.getLayer(TRANSPORT));

        if (ip_layer->getSrcIp() == cap_src_ip_ && tcp_layer->getFlags() == TH_SYN) {
            if (socket_ != -1) {
                // connection already exists ... 
                close(socket_);
                socket_ = -1;
             }

             capTcpState_ = SYN_SENT;
        } else if (tcp_layer->getDataSize()) {
            // unexpected packet 
        } else {
            // assume close handshake
        }
    }

    void IpV4TcpConversation::synSentProcessCapturePacket(const TransportPacket& packet) {
        IpLayer* ip_layer = dynamic_cast<IpLayer *>(packet.getLayer(NETWORK));
        TcpLayer* tcp_layer = dynamic_cast<TcpLayer *>(packet.getLayer(TRANSPORT));

        if (ip_layer->getSrcIp() == cap_dest_ip_ && tcp_layer->hasAck() && tcp_layer->hasSyn()) {
            capTcpState_ = SYN_RECEIVED;
        } else {
            // unexpected packet
        }
    }

    void IpV4TcpConversation::synRecvProcessCapturePacket(const TransportPacket& packet) {
        IpLayer* ip_layer = dynamic_cast<IpLayer *>(packet.getLayer(NETWORK));
        TcpLayer* tcp_layer = dynamic_cast<TcpLayer *>(packet.getLayer(TRANSPORT));

        if (ip_layer->getSrcIp() == cap_src_ip_ && tcp_layer->hasAck()) {
             capTcpState_ = ESTABLISHED;

             Action* connect_action = new Action(CONNECT);
    
             action_queue_.push(connect_action);
        } else {
            // unexpected packet
        }
    }

    void IpV4TcpConversation::estProcessCapturePacket(const TransportPacket& packet) {
        IpLayer* ip_layer = dynamic_cast<IpLayer *>(packet.getLayer(NETWORK));
        TcpLayer* tcp_layer = dynamic_cast<TcpLayer *>(packet.getLayer(TRANSPORT));

        int data_size = tcp_layer->getDataSize();
        if (data_size) {
            Action* action;
            if (ip_layer->getSrcIp() == cap_src_ip_ && tcp_layer->getSrcPort() == cap_src_port_) {
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
            capTcpState_ = CLOSED;

            action_queue_.push(new Action(CLOSE));
        }
    }
}