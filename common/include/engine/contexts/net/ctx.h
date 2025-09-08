//
// Created by niooi on 7/31/2025.
//

#pragma once

#include <defs.h>
#include <domain/context.h>
#include <engine/engine.h>
#include <engine/sync.h>
#include <entt/entt.hpp>
#include <fcntl.h>
#include <memory>
#include <string>
#include <unordered_dense.h>
#include "moodycamel/concurrentqueue.h"

#include "listener.h"

namespace v {
    enum ConnectionType { Incoming, Outgoing };

    typedef std::tuple<std::string, u16, ConnectionType> HostPortTypeTuple;
    typedef std::tuple<std::string, u16>                 HostPortTuple;
    typedef ENetPeer*                                    NetPeer;
    typedef ENetHost*                                    NetHost;

    class NetConnection;

    /// Network events that need to be processed on the main thread
    enum class NetworkEventType { NewConnection, ConnectionClosed };

    struct NetworkEvent {
        NetworkEventType               type;
        std::shared_ptr<NetConnection> connection;
        NetListener*                   server; // only used for NewConnection events
    };

    /// A context that creates and manages network connections.
    /// NetworkContext is thread-safe.
    class NetworkContext : public Context {
        friend NetConnection;
        friend NetListener;
        template <typename D, typename P>
        friend class NetChannel;

    public:
        /// update_every is the fixed update rate of the context's internal
        /// io loop, in seconds.
        explicit NetworkContext(Engine& engine, f64 update_every);
        ~NetworkContext();

        // TODO! return atomic counted intrusive handle
        // TODO! this kinda works for client to server but what abotu server accepting
        // clients? auto create connectiondomain??? probably. ConnectionDomain*
        /// Creates a new connection object that represents an outgoing connection.
        /// @note connection_timeout's max value is 5, it will not extend beyond 5.
        std::shared_ptr<NetConnection>
        create_connection(const std::string& host, u16 port, f64 connection_timeout=5.f);

        FORCEINLINE std::shared_ptr<NetConnection>
                    get_connection(const std::string& host, u16 port)
        {
            const auto maps = conn_maps_.read();
            const auto key  = std::make_tuple(host, port);

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
        {
            return nullptr;
        }

        std::shared_ptr<NetListener>
        listen_on(const std::string& addr, u16 port, u32 max_connections = 128);

        /// Handles network flushing and updating
        void update();

    private:
        // data is either a pointer to NetListener (for access to the callbacks)
        // or nothing.
        void update_host(NetHost host, void* data);

        /// Links peer connection info to conn_maps_ for bidirectional lookup
        void link_peer_conn_info(NetPeer peer, const std::string& host, u16 port);

        /// Links host server info to server_maps_ for bidirectional lookup
        void link_host_server_info(NetHost host, const std::string& addr, u16 port);

        /// Cleanup internal tracking of a connection
        FORCEINLINE void cleanup_tracking(NetPeer peer)
        {
            {
                auto conns = connections_.write();
                if (!conns->contains(peer))
                {
                    LOG_WARN(
                        "Requested close on connection that is not alive.. This should "
                        "not happen.");
                    return;
                }
                conns->erase(peer);
            }

            {
                // remove DeMap stuff that was linking info together
                auto maps = conn_maps_.write();

                if (maps->backward.contains(peer))
                {

                    auto tuple = maps->backward.at(peer);

                    maps->forward.erase(tuple);
                    maps->backward.erase(peer);
                }
            }
        }


        /// Rate at which the internal io loop polls stuff, in seconds
        /// TODO if this is ever modifable in the future, make atomic
        f64              update_rate_;
        std::thread      io_thread_;
        std::atomic_bool is_alive_{ true };

        // for outgoing and incoming connections
        RwLock<ankerl::unordered_dense::map<NetPeer, std::shared_ptr<NetConnection>>>
            connections_{};

        // for listeners
        // TODO! is this bad? do i have an illness?
        RwLock<ankerl::unordered_dense::map<NetHost, std::shared_ptr<NetListener>>>
            servers_{};

        // literally just two maps
        template <typename K, typename T>
        struct DeMap {
            ankerl::unordered_dense::map<K, T> forward{};

            ankerl::unordered_dense::map<T, K> backward{};
        };

        // for outgoing connection management
        RwLock<DeMap<HostPortTuple, ENetPeer*>> conn_maps_{};

        // for listener management
        RwLock<DeMap<HostPortTuple, ENetHost*>> server_maps_{};

        /// An ENetHost object whose sole purpose is to manage outgoing connections
        /// (listening on a port needs its own host object)
        RwLock<ENetHost*> outgoing_host_;

        /// Queue for network events that need to be processed on the main thread
        moodycamel::ConcurrentQueue<NetworkEvent> event_queue_;
    };
} // namespace v
