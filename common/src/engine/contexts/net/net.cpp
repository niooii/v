//
// Created by niooi on 7/31/2025.
//

#define ENET_IMPLEMENTATION
#include <defs.h>
#include <engine/contexts/net/net.h>
#include <stdexcept>

namespace v {
    NetworkContext::NetworkContext(Engine& engine) : Context(engine)
    {

        if (enet_initialize() != 0)
        {
            throw std::runtime_error("Failed to initialize ENet");
        }
    }

    NetworkContext::~NetworkContext()
    {
        enet_deinitialize();
    }

    void NetworkContext::update() {
        auto conns = connections_.write();

        for (auto& [a, b] : *conns) {
            // TODO!
            ENetHost* host = b->enet_host_;
            // shuttle packets back and forth here
        }
    }

} // namespace v
