#include <string.h>

#include "action.h"
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
        auto udp_data = reinterpret_cast<const char *>(udp_layer->getData());

        if (memcmp(cap_src_addr_.get(), layer3->getSrcAddr(), addr_size_) == 0 && cap_src_port_ == udp_layer->getSrcPort()) {
            action = new Action(Action::Type::SEND, udp_data, udp_layer->getDataSize());
        } else {
            action = new Action(Action::Type::RECV, udp_data, udp_layer->getDataSize());
        }

        action_queue_.push_back(action);
    }    
} // namespace packet_reply

