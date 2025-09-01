//
// Created by niooi on 7/31/2025.
//

#pragma once

#include <absl/debugging/internal/demangle.h>
#include <defs.h>
#include <domain/context.h>
#include <engine/contexts/net/connection.h>
#include <engine/sync.h>
#include <entt/entt.hpp>
#include <fcntl.h>
#include <memory>
#include <string>
#include "entt/entity/fwd.hpp"
#include "unordered_dense.h"

namespace v {
    typedef std::tuple< std::string, u16> ConnKey;

    // TODO! think about how to design this for fast use from multiple threads.

    /// A context that creates and manages network connections.
    /// NetworkContext is thread-safe.
    class NetworkContext : public Context {
        friend NetConnection;

    public:
        explicit NetworkContext(Engine& engine);
        ~NetworkContext();

        // TODO! return atomic counted intrusive handle
        // TODO! this kinda works for client to server but what abotu server accepting
        // clients? auto create connectiondomain??? probably. ConnectionDomain*
        /// Creates a new connection object. 
        FORCEINLINE std::shared_ptr<NetConnection>
                    create_connection(const std::string& host, u16 port, std::optional<HostOptions> host_opts = std::nullopt)
        {
            auto key = std::make_tuple(host, port);
            {
                auto conn_read = connections_.read();
                if (conn_read->contains(key))
                    return conn_read->at(key);
            }
            auto res = connections_.write()->emplace(
                key, NetConnection{this, host, port, host_opts});
            return res.first->second;
        }

        FORCEINLINE std::shared_ptr<NetConnection>
                    get_connection(const std::string& host, u16 port)
        {
            auto key       = std::make_tuple(host, port);
            auto conn_read = connections_.read();

            auto it = conn_read->find(key);
            if (it != conn_read->end())
                return { it->second };
            else
                return nullptr;
        }

        /// Fires when a new connection is creataed.
        // entt::sink<entt::sigh<void(DomainId)>> on_conn;

        /// Handles network flushing and updating
        void update();

    private:
        // Don't use the main engine domain registry.
        RWProtectedResource<entt::registry> reg_{};

        RWProtectedResource<ankerl::unordered_dense::map<
            ConnKey, std::shared_ptr<NetConnection>>>
            connections_{};
    };
} // namespace v
