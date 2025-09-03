//
// Created by niooi on 7/31/2025.
//

#pragma once

#include <defs.h>
#include <domain/context.h>
#include <engine/sync.h>
#include <entt/entt.hpp>
#include <fcntl.h>
#include <memory>
#include <string>
#include <unordered_dense.h>

#include "connection.h"
#include "listener.h"

namespace v {
    typedef ENetPeer*                    NetPeer;
    typedef std::tuple<std::string, u16> HostPortTuple;

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
        /// Creates a new connection object that represents an outgoing connection.
        FORCEINLINE std::shared_ptr<NetConnection>
                    create_connection(const std::string& host, u16 port)
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

        FORCEINLINE std::shared_ptr<NetConnection>
                    get_connection(const std::string& host, u16 port)
        {
            const auto maps = hip_map_.read();
            const auto key  = std::make_tuple(host, port);

            const auto it = maps->host_to_peer_.find(key);

            if (LIKELY(it != maps->host_to_peer_.end()))
                return get_connection(it->second);
            else
                return nullptr;
        }

        FORCEINLINE std::shared_ptr<NetConnection> get_connection(NetPeer peer)
        {
            auto conn_read = connections_.read();

            auto it = conn_read->find(peer);
            if (it != conn_read->end())
                return { it->second };
            else
                return nullptr;
        }

        FORCEINLINE std::shared_ptr<NetConnection>
                    incoming_connection(const std::string& addr)
        {}

        FORCEINLINE std::shared_ptr<NetListener> listen_on(const std::string& addr, u16 port) {
            
        }

        /// Fires when a new connection is creataed.
        // entt::sink<entt::sigh<void(DomainId)>> on_conn;

        /// Handles network flushing and updating
        void update();

    private:
        // Don't use the main engine domain registry.
        RWProtectedResource<entt::registry> reg_{};

        RWProtectedResource<
            ankerl::unordered_dense::map<NetPeer, std::shared_ptr<NetConnection>>>
            connections_{};

        struct HostInfoPeerMapping {
            // the classic dual map pattern
            ankerl::unordered_dense::map<HostPortTuple, NetPeer> host_to_peer_{};

            ankerl::unordered_dense::map<NetPeer, HostPortTuple> peer_to_host_{};
        };

        RWProtectedResource<HostInfoPeerMapping> hip_map_{};

        /// An ENetHost object whose sole purpose is to manage outgoing connections
        /// (listening on a port needs its own host object)
        ENetHost* outgoing_;
    };
} // namespace v
