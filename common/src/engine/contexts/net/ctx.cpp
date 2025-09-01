//
// Created by niooi on 7/31/2025.
//

#define ENET_IMPLEMENTATION
#include <defs.h>
#include <engine/contexts/net/ctx.h>
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
        // abusing interior mutability - this is fine
        // as long as we keep the netconnection class
        // thread safe
        auto conns = connections_.read();

        for (auto& [_a, b] : *conns) {
            b->update();
        }
    }

} // namespace v
