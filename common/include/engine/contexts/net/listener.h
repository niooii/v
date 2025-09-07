//
// Created by niooi on 7/31/2025.
//

#pragma once

#include <defs.h>
#include <enet.h>
#include <entt/entt.hpp>
#include <unordered_dense.h>

namespace v {
    class NetConnection;
    typedef std::function<void(std::shared_ptr<NetConnection>)> OnConnectCallback;
    typedef std::function<void(std::shared_ptr<NetConnection>)> OnDisconnectCallback;

    struct ServerComponent {
        /// Set a callback to run when a new incoming connection is created.
        OnConnectCallback on_connect{};
        /// If new_only is false, the on_connect callback will run immediately for
        /// all the incoming connections that already exist.
        /// If new_only is true, the callback will only run for *future* connections.
        /// new_only is false by default.
        // This is internally set to true once all the old connections have been processed, btw.
        bool new_only{};

        /// Set a callback to run when an incoming connection has been disconnected.
        /// The connection will most likely be dead.
        OnDisconnectCallback on_disconnect{};
    };

    /// A 'server' class.
    class NetListener {
        friend class NetworkContext;

    public:
        /// Creates and attaches a ServerComponent to an entity, usually a Domain
        ServerComponent& create_component(entt::entity id);

    private:
        NetListener(
            class NetworkContext* ctx, const std::string& host, u16 port,
            u32 max_connections = 128);

        // called by net context when new connection is inbound
        void handle_new_connection(std::shared_ptr<NetConnection> con);

        /// Function will update server components by calling them on all previous connections if requested
        void update();

        std::string addr_;
        u16         port_;

        NetworkContext* net_ctx_;
        ENetHost*       host_;

        ankerl::unordered_dense::set<ENetPeer*> connected_;
    };
} // namespace v
