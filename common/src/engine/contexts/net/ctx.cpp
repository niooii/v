//
// Created by niooi on 7/31/2025.
//

#include <vector>
#define ENET_IMPLEMENTATION
#include <defs.h>
#include <engine/contexts/net/ctx.h>
#include <stdexcept>
#include <time/stopwatch.h>
#include "enet.h"
#include "engine/contexts/net/connection.h"
#include <engine/contexts/net/channel.h>
#include "engine/contexts/net/listener.h"
#include "engine/sync.h"
#include "time/time.h"

namespace v {
    NetworkContext::NetworkContext(Engine& engine, f64 update_every) :
        Context(engine), update_rate_(update_every)
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

        // start io thread
        io_thread_ = std::thread(
            [this]()
            {
                Stopwatch stopwatch{};
                while (is_alive_)
                {
                    stopwatch.reset();

                    {
                        auto host = outgoing_host_.write();
                        update_host(*host, NULL);
                    }
                    {
                        auto servers = servers_.write();
                        for (auto& [host, listener] : *servers)
                        {
                            update_host(host, (void*)listener.get());
                        }
                    }

                    f64 sleep_time = stopwatch.until(update_rate_);
                    if (sleep_time > 0)
                        v::time::sleep_ns(static_cast<u64>(sleep_time * 1e9));
                }
            });
    }

    NetworkContext::~NetworkContext()
    {
        is_alive_ = false;
        io_thread_.join();
        enet_deinitialize();
    }


    std::shared_ptr<NetConnection> NetworkContext::create_connection(
        const std::string& host, u16 port, f64 connection_timeout)
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

        auto con = std::shared_ptr<NetConnection>(
            new NetConnection(this, host, port, connection_timeout));
        con->shared_con_ = con;
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

        auto server = std::shared_ptr<NetListener>(
            new NetListener(this, addr, port, max_connections));
        auto res = servers_.write()->emplace(server->host_, server);
        link_host_server_info(server->host_, addr, port);
        return res.first->second;
    }

    void NetworkContext::update_host(NetHost host, void* data)
    {
        ENetEvent event;

        while (enet_host_service(host, &event, 0) > 0)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_CONNECT:
                {
                    if (event.peer->data)
                    {
                        // the connection object already exists, and therefore is an
                        // outgoing connection - queue activation event
                        auto con = static_cast<NetConnection*>(event.peer->data);

                        NetworkEvent activate_event{
                            .type       = NetworkEventType::ActivateConnection,
                            .connection = con->shared_con_,
                        };
                        event_queue_.enqueue(std::move(activate_event));
                        break;
                    }

                    // atp we know the host is a server host
                    NetListener* server = static_cast<NetListener*>(data);

                    auto con = std::shared_ptr<NetConnection>(
                        new NetConnection(this, event.peer));
                    con->shared_con_ = con;

                    auto res = connections_.write()->emplace(
                        const_cast<ENetPeer*>(con->peer()), con);

                    NetworkEvent activate_event{
                        .type       = NetworkEventType::ActivateConnection,
                        .connection = con,
                    };
                    event_queue_.enqueue(std::move(activate_event));

                    NetworkEvent event{
                        .type       = NetworkEventType::NewConnection,
                        .connection = con,
                        .server     = server,
                    };
                    event_queue_.enqueue(std::move(event));
                }
                break;
            case ENET_EVENT_TYPE_RECEIVE:
                {
                    NetPeer peer = event.peer;
                    auto    con  = (NetConnection*)(peer->data);
                    con->handle_raw_packet(event.packet);
                }
                break;

            // this event is generated when we call enet_peer_disconnect.
            // in the case it was disconnected remotely, we still have to remove
            // the internal tracking state
            case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT:
                LOG_ERROR("Connection timed out.");
            case ENET_EVENT_TYPE_DISCONNECT:
                NetPeer peer = event.peer;
                // if data is NULL, then we triggered the disconnect locally
                if (!peer->data)
                {
                    break;
                }

                // otherwise the disconnect was triggered remotely, and our connection
                // is still valid. Queue connection close event for main thread processing
                auto con = (NetConnection*)(peer->data);

                con->remote_disconnected_ = true;

                if (con->shared_con_)
                {
                    // pass server info if this disconnect came from a server host
                    NetListener* server = static_cast<NetListener*>(data);
                    NetworkEvent event{
                        .type       = NetworkEventType::ConnectionClosed,
                        .connection = con->shared_con_,
                        .server     = server,
                    };
                    event_queue_.enqueue(std::move(event));
                }
                break;
            }
        }
    }

    void NetworkContext::update()
    {
        // process queued network events from IO thread first
        NetworkEvent event;
        while (event_queue_.try_dequeue(event))
        {
            switch (event.type)
            {
            case NetworkEventType::ActivateConnection:
                if (event.connection)
                {
                    event.connection->activate_connection();
                }
                break;

            case NetworkEventType::DestroyConnection:
                if (event.connection)
                {
                    cleanup_tracking(const_cast<ENetPeer*>(event.connection->peer()));

                    auto lock = event.connection->map_lock_.write();
                    // delete all channel pointers
                    for (auto& [a, b] : event.connection->c_insts_)
                        delete b;

                    // remove all the channel components we connected to it (the pointers are dead now)
                    engine_.registry().destroy(event.connection->entity_);

                    event.connection->c_insts_.clear();
                    event.connection->recv_c_ids_.clear();
                    event.connection->recv_c_info_.clear();

                    event.connection->shared_con_.reset();
                }
                break;

            case NetworkEventType::NewConnection:
                if (event.server)
                    event.server->handle_new_connection(event.connection);

                break;

            case NetworkEventType::ConnectionClosed:
                if (event.connection)
                {
                    // Call disconnection handler if this was a server connection
                    if (event.server)
                        event.server->handle_disconnection(event.connection);

                    NetworkEvent destroy_event{
                        .type       = NetworkEventType::DestroyConnection,
                        .connection = event.connection,
                    };
                    event_queue_.enqueue(std::move(destroy_event));
                }
                break;

            case NetworkEventType::ChannelLink:
                auto& c_insts = event.connection->c_insts_;
                auto  inst    = c_insts.find(event.created_channel.name);

                {
                    auto  lock = event.connection->map_lock_.write();
                    auto& info = event.connection->recv_c_info_.at(
                        event.created_channel.remote_uid);

                    LOG_DEBUG("lf channel name locally: {}.", event.created_channel.name);

                    if (inst != c_insts.end())
                    {
                        // the channel instance already exists locally
                        info.channel = inst->second;
                    }
                    else
                    {
                        LOG_DEBUG("could not find {} locally", event.created_channel.name);
                        if (!info.before_creation_packets)
                            info.before_creation_packets = new moodycamel::ConcurrentQueue<ENetPacket*>();
                    }
                }
                break;
            }
        }

        // Update server stuff
        for (auto& [host, server] : *servers_.read())
        {
            server->update();
        }

        std::vector<std::shared_ptr<NetConnection>>* to_close{ nullptr };

        // Update all active connections
        for (auto& [peer, net_con] : *connections_.read())
        {
            NetConnectionResult res = net_con->update();

            if (UNLIKELY(res == NetConnectionResult::TimedOut))
            {
                if (!to_close)
                    to_close = new std::vector<std::shared_ptr<NetConnection>>();
                to_close->push_back(net_con);
            }
        }

        // close timed out connection attempts
        if (to_close)
        {
            for (auto& net_con : *to_close)
            {
                net_con->request_close();
            }

            delete to_close;
        }
    }

    void
    NetworkContext::link_peer_conn_info(NetPeer peer, const std::string& host, u16 port)
    {
        const auto key       = std::make_tuple(host, port);
        auto       maps      = conn_maps_.write();
        maps->forward[key]   = peer;
        maps->backward[peer] = key;
    }

    void
    NetworkContext::link_host_server_info(NetHost host, const std::string& addr, u16 port)
    {
        const auto key       = std::make_tuple(addr, port);
        auto       maps      = server_maps_.write();
        maps->forward[key]   = host;
        maps->backward[host] = key;
    }

} // namespace v
