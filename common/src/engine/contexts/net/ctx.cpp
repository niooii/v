//
// Created by niooi on 7/31/2025.
//

#include "enet.h"
#include "engine/contexts/net/connection.h"
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

        new (&outgoing_) RWProtectedResource<ENetHost*>(host);
    }

    NetworkContext::~NetworkContext() { enet_deinitialize(); }


    std::shared_ptr<NetConnection>
                NetworkContext::create_connection(const std::string& host, u16 port)
    {
        const auto key = std::make_tuple(host, port);
        {
            const auto maps = hip_map_.read();
            const auto it   = maps->host_to_peer_.find(key);

            if (UNLIKELY(it != maps->host_to_peer_.end()))
            {
                NetPeer peer = it->second;
                return connections_.read()->at(peer);
            }
        }

        auto con = NetConnection{ this, host, port };
        auto res = connections_.write()->emplace(con.peer(), std::move(con));
        return res.first->second;
    }

    void NetworkContext::update()
    {
        // update outgoing connection stuff
        {
            ENetEvent event;
            auto      host = outgoing_.write();

            while (enet_host_service(*host, &event, 0) > 0)
            {
                switch (event.type)
                {
                case ENET_EVENT_TYPE_RECEIVE:
                    {
                        NetPeer peer = event.peer;
                        auto con = (NetConnection*)(peer->data);
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

                    // otherwise the disconnect was triggered remotely, and our connection is still valid. remove internal tracking stuff
                    req_close(peer);
                    break;
                }
            }
        }


        // abusing interior mutability - this is fine
        // as long as we keep the netconnection class
        // thread safe
        // auto conns = connections_.read();

        // for (auto& [_a, b] : *conns)
        // {
        //     b->update();
        // }
    }

} // namespace v
