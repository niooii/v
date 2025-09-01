//
// Created by niooi on 7/31/2025.
//

#pragma once

#include <absl/debugging/internal/demangle.h>
#include <defs.h>
#include <domain/context.h>
#include <engine/contexts/net/net_connection.h>
#include <engine/sync.h>
#include <entt/entt.hpp>
#include <fcntl.h>
#include <memory>
#include <string>
#include "entt/entity/fwd.hpp"
#include "unordered_dense.h"

namespace v {
    // TODO! think about how to design this for fast use from multiple threads.

    /// A context that creates and manages network connections.
    /// NetworkContext is thread-safe.
    class NetworkContext : public Context {
        friend NetConnection;

    public:
        explicit NetworkContext(Engine& engine);
        ~NetworkContext();

        FORCEINLINE NetConnection* create_connection(const std::string& host, u16 port)
        {
            auto key = std::make_tuple(host, port);
            auto res = connections_.write()->emplace(key, NetConnection::create(this, host, port));
            return res.first->second.get();
        }

        /// Fires when a new connection is creataed.
        // entt::sink<entt::sigh<void(DomainId)>> on_conn;

        // TODO! this kinda works for client to server but what abotu server accepting
        // clients? auto create connectiondomain??? probably. ConnectionDomain*
        NetConnection* create_connection(const std::string_view host_ip, u16 port);

        /// Handles network flushing and updating
        void update();

    private:
        // Don't use the main engine domain registry.
        RWProtectedResource<entt::registry> reg_{};

        RWProtectedResource<ankerl::unordered_dense::map<
            std::tuple<std::string, u16>, std::unique_ptr<NetConnection>>>
            connections_{};
    };
} // namespace v
