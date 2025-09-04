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

#include "listener.h"

namespace v {
    enum ConnectionType {
        Incoming,
        Outgoing
    };

    typedef std::tuple<std::string, u16, ConnectionType> HostPortTypeTuple;
    typedef std::tuple<std::string, u16> HostPortTuple;
    typedef ENetPeer*                    NetPeer;
    typedef ENetHost*                    NetHost;

    // TODO! think about how to design this for fast use from multiple threads.
    class NetConnection;

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
        std::shared_ptr<NetConnection>
        create_connection(const std::string& host, u16 port);

        FORCEINLINE std::shared_ptr<NetConnection>
                    get_connection(const std::string& host, u16 port)
        {
            const auto maps = conn_maps_.read();
            const auto key  = std::make_tuple(host, port, ConnectionType::Outgoing);

            const auto it = maps->forward.find(key);

            if (LIKELY(it != maps->forward.end()))
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

        std::shared_ptr<NetListener>
                    listen_on(const std::string& addr, u16 port, u32 max_connections=128);

        /// Request a close to the connection via its peer.
        /// Other functions can no longer access this connection, but
        /// the connection will remain alive until the last instance of it's handle
        /// is gone.
        FORCEINLINE void req_close(NetPeer peer)
        {
            {
                // if this exists then we can close the connection
                const auto maps = conn_maps_.read();
                const auto it   = maps->backward.find(peer);
                if (it == maps->backward.end())
                {
                    LOG_WARN(
                        "Requested close on connection that is not alive.. This should "
                        "not happen.");
                }
            }

            {
                auto maps  = conn_maps_.write();
                auto tuple = maps->backward.at(peer);

                maps->forward.erase(tuple);
                maps->backward.erase(peer);
            }

            {
                auto conns = connections_.write();
                conns->erase(peer);
            }
        }


        /// Fires when a new connection is creataed.
        // entt::sink<entt::sigh<void(DomainId)>> on_conn;

        /// Handles network flushing and updating
        void update();

    private:
        // data is either a pointer to NetListener (for access to the callbacks)
        // or nothing. 
        void update_host(NetHost host, void* data);

        // Don't use the main engine domain registry.
        RWProtectedResource<entt::registry> reg_{};

        // for outgoing and incoming connections
        RWProtectedResource<
            ankerl::unordered_dense::map<NetPeer, std::shared_ptr<NetConnection>>>
            connections_{};

        // for listeners
        // TODO! is this bad? do i have an illness?
        RWProtectedResource<
            ankerl::unordered_dense::map<NetHost, std::shared_ptr<NetListener>>>
            servers_{};

        // literally just two maps
        template <typename K, typename T>
        struct DeMap {
            ankerl::unordered_dense::map<K, T> forward{};

            ankerl::unordered_dense::map<T, K> backward{};
        };

        // for outgoing connection management
        RWProtectedResource<DeMap<HostPortTypeTuple, ENetPeer*>> conn_maps_{};

        // for listener management
        RWProtectedResource<DeMap<HostPortTuple, ENetHost*>> server_maps_{};

        /// An ENetHost object whose sole purpose is to manage outgoing connections
        /// (listening on a port needs its own host object)
        RWProtectedResource<ENetHost*> outgoing_host_;
    };
} // namespace v
