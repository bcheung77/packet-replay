#ifndef PACKET_REPLAY_TRANSPORT_PACKET_H
#define PACKET_REPLAY_TRANSPORT_PACKET_H

#include <vector>

#include "network_layers.h"

namespace packet_replay {
    /**
     * A packet encapsulation with data split into layers 1 - 4.
     */
    class TransportPacket
    {
        private:
            std::vector<Layer*> layers_;
        public:
            TransportPacket(const TransportPacket&) = delete;
            TransportPacket& operator=(const TransportPacket&) = delete;
            TransportPacket() : layers_(TRANSPORT, nullptr) {
            }

            ~TransportPacket() {
                for (auto layer : layers_) {
                    if (layer != nullptr) {
                        delete layer;
                    }
                }
            }

            void addLayer(Layer* layer) {
                layers_[layer->getLayerNumber() - 1] = layer;
            }

            Layer* getLayer(LayerNumber num) const {
                return layers_[num - 1];
            }

            bool isLayer(LayerNumber num, Protocol proto) const {
                Layer* layer = getLayer(num);
                return layer != nullptr && layer->getProtocol() == proto;
            }
    };    

}

#endif