//
// Created by niooi on 8/1/2025.
//

#include <engine/contexts/net/connection.h>
#define ENET_IMPLEMENTATION
#include <defs.h>
#include <enet.h>
#include <engine/contexts/net/ctx.h>
#include <stdexcept>

namespace v {
    // create an outgoing connection
    NetConnection::NetConnection(NetworkContext* ctx, const std::string& host, u16 port) :
        net_ctx_(ctx), conn_type_(ConnectionType::Outgoing)
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

        peer_ = enet_host_connect(
            *ctx->outgoing_host_.write(), &address, 4 /* 4 channels */, 0);

        if (!peer_)
        {
            LOG_ERROR("Failed to connect to peer at {}:{}", host, port);
            throw std::runtime_error("bleh");
        }

        peer_->data = (void*)this;
    }

    // just an incoming connection, its whatever
    NetConnection::NetConnection(NetworkContext* ctx, ENetPeer* peer) :
        net_ctx_(ctx), peer_(peer), conn_type_(ConnectionType::Incoming)
    {}

    // at this point there should be no more references internally
    NetConnection::~NetConnection()
    {
        // internally queues disconnect
        if (!remote_disconnected_)
            enet_peer_disconnect(peer_, 0);

        // make it known that the connection was disconnected locally
        peer_->data = NULL;
    }

    void NetConnection::handle_raw_packet(ENetPacket* packet)
    {
        // TODO! auto channel creation, routing, etc
        LOG_DEBUG("got packet yep");
    }
} // namespace v
