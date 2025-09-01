//
// Created by niooi on 8/1/2025.
//

#define ENET_IMPLEMENTATION
#include <defs.h>
#include <engine/contexts/net/net.h>
#include <stdexcept>

namespace v {
    NetConnection::NetConnection(NetworkContext* ctx, const std::string& host, u16 port) :
        host_(host), port_(port), net_ctx_(ctx)
    {
        entity_ = ctx->reg_.write()->create();
        
        // TODO! init enet host and other stuff
        ENetAddress address;
        if (enet_address_set_host(&address, host_.c_str()) != 0) {
            enet_deinitialize();
            throw std::runtime_error("Failed to set host address");
        }
        address.port = port;

        enet_host_ = enet_host_create(&address, 32, 2, 0, 0);
        if (enet_host_ == nullptr) {
            enet_deinitialize();
            throw std::runtime_error("Failed to create ENet host");
        }
    }

    NetConnection::~NetConnection() {
        if (enet_host_ != nullptr)
        {
            enet_host_destroy(enet_host_);
        }
    }
}
