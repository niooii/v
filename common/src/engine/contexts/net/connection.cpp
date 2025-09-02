//
// Created by niooi on 8/1/2025.
//

#include "engine/contexts/net/connection.h"
#include "enet.h"
#define ENET_IMPLEMENTATION
#include <defs.h>
#include <engine/contexts/net/ctx.h>
#include <stdexcept>

namespace v {
    // create an outgoing connection
    NetConnection::NetConnection(NetworkContext* ctx, const std::string& host, u16 port) :
        net_ctx_(ctx)
    {
        entity_ = ctx->reg_.write()->create();

        // TODO! init enet host and other stuff
        ENetAddress address;
        if (enet_address_set_host(&address, host.c_str()) != 0)
        {
            enet_deinitialize();
            throw std::runtime_error("Failed to set host address");
        }
        address.port = port;

        enet_host_ = enet_host_create(&address, 32, 2, 0, 0);
        if (enet_host_ == nullptr)
        {
            enet_deinitialize();
            throw std::runtime_error("Failed to create ENet host");
        }
    }

    // just an incoming connection, its whatever
    NetConnection::NetConnection(NetworkContext* ctx, ENetPeer* peer) : net_ctx_(ctx), peer_(peer) {
        
    }

    NetConnection::~NetConnection()
    {
        if (enet_host_ != nullptr)
        {
            enet_host_destroy(enet_host_);
        }
    }

    void NetConnection::update()
    {
        // TODO! auto channel creation etc
        // for now just update the enet thing

        ENetEvent event;
        while (enet_host_service(enet_host_, &event, 0) > 0)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_CONNECT:
                break;

            case ENET_EVENT_TYPE_RECEIVE:
                break;

            case ENET_EVENT_TYPE_DISCONNECT:
                break;
            }
        }
    }
} // namespace v
