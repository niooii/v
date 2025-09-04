//
// Created by niooi on 7/31/2025.
//

#include "enet.h"
#include "engine/contexts/net/connection.h"
#include "engine/contexts/net/listener.h"
#include "engine/sync.h"
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

        // more than 250 then you should not be game programming..
        constexpr u16 MAX_OUTGOING = 250;
        ENetHost*     host         = enet_host_create(
            NULL, MAX_OUTGOING, 4 /* 4 channels */,
            0 /* any amount of incoming bandwidth */,
            0 /* any amount of incoming bandwidth */
        );

        if (!host)
        {
            LOG_CRITICAL("Failed to create net client host..");
            throw std::runtime_error("bleh");
        }

        new (&outgoing_host_) RwLock<ENetHost*>(host);
    }

    NetworkContext::~NetworkContext() { enet_deinitialize(); }


    std::shared_ptr<NetConnection>
    NetworkContext::create_connection(const std::string& host, u16 port)
    {
        const auto key = std::make_tuple(host, port);
        {
            const auto maps = conn_maps_.read();
            const auto it   = maps->forward.find(key);

            if (UNLIKELY(it != maps->forward.end()))
            {
                NetPeer peer = it->second;
                return connections_.read()->at(peer);
            }
        }

        auto con = std::shared_ptr<NetConnection>(new NetConnection(this, host, port));
        auto res = connections_.write()->emplace(const_cast<ENetPeer*>(con->peer()), con);

        link_peer_conn_info(const_cast<ENetPeer*>(con->peer()), host, port);
        return res.first->second;
    }

    std::shared_ptr<NetListener>
    NetworkContext::listen_on(const std::string& addr, u16 port, u32 max_connections)
    {
        const auto key = std::make_tuple(addr, port);
        {
            const auto maps = server_maps_.read();
            const auto it   = maps->forward.find(key);

            if (UNLIKELY(it != maps->forward.end()))
            {
                NetHost host = it->second;
                return servers_.read()->at(host);
            }
        }

        auto server = std::shared_ptr<NetListener>(new NetListener(this, addr, port, max_connections));
        auto res    = servers_.write()->emplace(server->host_, server);
        link_host_server_info(server->host_, addr, port);
        return res.first->second;
    }

    void NetworkContext::update_host(NetHost host, void* data) {
        ENetEvent event;

        while (enet_host_service(host, &event, 0) > 0)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_CONNECT:
                {
                    // atp we know the host is a server host
                    NetListener* server = static_cast<NetListener*>(data);

                    auto con = std::shared_ptr<NetConnection>(new NetConnection(this, event.peer));
                    auto res = connections_.write()->emplace(const_cast<ENetPeer*>(con->peer()), con);

                    // Skip DeMap population for incoming connections to avoid NAT collision issues

                    server->handle_new_connection(con);
                }
                break;
            case ENET_EVENT_TYPE_RECEIVE:
                {
                    NetPeer peer = event.peer;
                    auto    con  = (NetConnection*)(peer->data);
                    con->handle_raw_packet(event.packet);
                    enet_packet_destroy(event.packet);
                }
                break;

            // this event is generated when we call enet_peer_disconnect.
            // in the case it was disconnected remotely, we still have to remove
            // the internal tracking state
            case ENET_EVENT_TYPE_DISCONNECT:
            case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT:
                NetPeer peer = event.peer;
                // if data is NULL, then we disconnected locally.
                if (!peer->data)
                    break;

                // otherwise the disconnect was triggered remotely, and our connection
                // is still valid. remove internal tracking stuff
                req_close(peer);
                break;
            }
        }
    }

    void NetworkContext::update()
    {
        // update outgoing connection stuff
        {
            ENetEvent event;
            auto      host = outgoing_host_.write();

            while (enet_host_service(*host, &event, 0) > 0)
            {
                switch (event.type)
                {
                case ENET_EVENT_TYPE_RECEIVE:
                    {
                        NetPeer peer = event.peer;
                        auto    con  = (NetConnection*)(peer->data);
                        con->handle_raw_packet(event.packet);
                        enet_packet_destroy(event.packet);
                    }
                    break;

                // this event is generated when we call enet_peer_disconnect.
                // in the case it was disconnected remotely, we still have to remove
                // the internal tracking state
                case ENET_EVENT_TYPE_DISCONNECT:
                case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT:
                    NetPeer peer = event.peer;
                    // if data is NULL, then we disconnected locally.
                    if (!peer->data)
                        break;

                    // otherwise the disconnect was triggered remotely, and our connection
                    // is still valid. remove internal tracking stuff
                    req_close(peer);
                    break;
                }
            }
        }


        // abusing interior mutability - this is fine
        // as long as we keep the netconnection class
        // thread safe
        auto conns = servers_.read();


    }

    void NetworkContext::link_peer_conn_info(NetPeer peer, const std::string& host, u16 port)
    {
        const auto key = std::make_tuple(host, port);
        auto maps = conn_maps_.write();
        maps->forward[key] = peer;
        maps->backward[peer] = key;
    }

    void NetworkContext::link_host_server_info(NetHost host, const std::string& addr, u16 port)
    {
        const auto key = std::make_tuple(addr, port);
        auto maps = server_maps_.write();
        maps->forward[key] = host;
        maps->backward[host] = key;
    }

} // namespace v
