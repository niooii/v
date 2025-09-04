//
// Created by niooi on 7/31/2025.
//

#pragma once

#include <defs.h>
#include <enet.h>
#include <unordered_dense.h>
#include <entt/entt.hpp>

namespace v {
    class NetConnection;
    typedef std::function<void(std::shared_ptr<NetConnection>)> OnConnectCallback;

    struct ServerComponent {
        /// Set a callback to run when a new incoming connection is created.
        OnConnectCallback on_connect{};
        /// If new_only is false, the callback will run immediately for
        /// all the incoming connections that already exist.
        /// If new_only is true, the callback will only run for *future* connections.
        /// new_only is false by default.
        bool new_only{};
    };
    
    /// A 'server' class.
    class NetListener {
        friend class NetworkContext;

    public:
        /// Creates an empty ServerComponent for attaching callbacks to
        FORCEINLINE ServerComponent create_component() { return {}; };

        /// Attaches a RenderComponent to an entity, usually a Domain
        /// TODO! should each net chanenl be its own context...?
        /// since i wanna keep this pattern of creating a compoennt and attaching it
        /// for calllbacks
        /// TODO! decide if they use the main domain registry or altenrate (the net one specifically) - bottleneck?
        void attach_component(const ServerComponent& component, entt::entity id);

    private:
        NetListener(
            class NetworkContext* ctx, const std::string& host, u16 port,
            u32 max_connections = 128);

        // called by net context when new connection is inbound
        void handle_new_connection(std::shared_ptr<NetConnection> con);

        std::string addr_;
        u16         port_;

        NetworkContext* net_ctx_;
        ENetHost*       host_;

        ankerl::unordered_dense::set<ENetPeer*> connected_;

        std::vector<OnConnectCallback> conn_callbacks_;
    };
} // namespace v
