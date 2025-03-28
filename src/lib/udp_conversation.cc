#include <string.h>

#include "transport_packet.h"
#include "udp_conversation.h"

namespace packet_replay
{
    void UdpConversation::processCapturePacket(const TransportPacket& packet) {
        UdpLayer* udp_layer = dynamic_cast<UdpLayer*>(packet.getLayer(TRANSPORT));
        Layer3* layer3 = dynamic_cast<Layer3*>(packet.getLayer(NETWORK));

        if (udp_layer == nullptr) {
            // not udp packet
            return;
        }

        Action* action;
        if (memcmp(cap_src_addr_, layer3->getSrcAddr(), addr_size_) == 0 && cap_src_port_ == udp_layer->getSrcPort()) {
            action = new Action(SEND);
        } else {
            action = new Action(RECV);
        }

        auto data_size = udp_layer->getDataSize();
        action->data_ = new uint8_t[data_size];
        memcpy(action->data_, udp_layer->getData(), data_size);
        action->data_size_ = data_size;

        action_queue_.push(action);
    }    
} // namespace packet_reply

