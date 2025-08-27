//
// Created by niooi on 7/31/2025.
//

#pragma once

#include <absl/debugging/internal/demangle.h>
#include <defs.h>
#include <domain/context.h>
#include <engine/sync.h>
#include <entt/entt.hpp>
#include <fcntl.h>
#include <string>
#include "entt/entity/fwd.hpp"
#include "unordered_dense.h"

extern "C" {
#include <enet.h>
}

namespace v {
    // TODO! think about how to design this for fast use from multiple threads.

    /// An abstract channel class, created and managed by a NetConnection connection
    /// object, meaning a channel is unique to a specific NetConnection only. To create a
    /// channel, inherit from this class.
    /// E.g. class ChatChannel : public NetChannel<ChatChannel> {};
    template <typename Derived>
    class NetChannel {
    public:
        // TODO! onrecv and send functions here, on send maybe?

        /// Get the owning NetConnection.
        FORCEINLINE const class NetConnection* connection_info() { return conn_; };

        /// Assigns a unique name to the channel, to be used for communication
        /// over a network.
        /// By default assigns the name of the struct and it's namespaces
        /// (e.g. "v::Chat::ChatChannel").
        /// Override this to set a new name that may be more consistent through codebase
        /// and version changes.
        /// TODO! find a way to enforce uniqueness for custom names, or just not allow
        /// inheritance
        constexpr virtual const std::string_view unique_name()
        {
            return type_name<Derived>();
        }

        void send(char* buf, u64 len);

    private:
        NetChannel(NetConnection* conn) : conn_(conn) {}

        const NetConnection* conn_;
    };

    template <typename T>
    concept DerivedFromChannel = std::is_base_of_v<NetChannel<T>, T>;

    class NetConnection {
        friend class NetworkContext;

    public:
        /// Creates a NetChannel for isolated network communication.
        /// If it already exists, it throws a warning and returns the one existing.
        /// TODO! This should automatically create a channel of the same type on the
        /// other side of the connection, yay! (so someone on the other end of the
        /// connection can wait on it)
        template <DerivedFromChannel T>
        NetChannel<T>* create_channel();

        /// Returns a NetChannel or nullptr if it doesn't exist.
        template <DerivedFromChannel T>
        NetChannel<T>* get_channel();

        /// Waits for the creation of a NetChannel (blocks thread), either
        /// locally or from the other end of the connection.
        /// TODO! how i gonna implement this lol
        template <DerivedFromChannel T>
        NetChannel<T>* wait_for_channel();

        FORCEINLINE const std::string& host() { return host_; }

        FORCEINLINE const u16 port() { return port_; }

    private:
        // TODO! change this to accept a Option<HostOptions> struct, if not provided then
        // it will not bind the host to any port/addr. (client usage, vs server usage
        // where you will pass in a host struct)
        NetConnection(class NetworkContext* ctx, const std::string& host, u16 port);
        ~NetConnection();

        ENetHost* enet_host_;

        const std::string host_;
        const u16         port_;
        entt::entity      entity_;

        // Pointer guarenteed to be alive here
        NetworkContext* net_ctx_;
    };

    /// A context that creates and manages network connections.
    /// NetworkContext is thread-safe.
    class NetworkContext : public Context {
        friend NetConnection;

    public:
        explicit NetworkContext(Engine& engine);
        ~NetworkContext();

        FORCEINLINE NetConnection* create_connection(const std::string& host, u16 port)
        {
            auto res = connections_.write()->emplace(host, port);
            return res.first->second.get();
        }

        /// Fires when a new connection is creataed.
        // entt::sink<entt::sigh<void(DomainId)>> on_conn;

        // TODO! this kinda works for client to server but what abotu server accepting
        // clients? auto create connectiondomain??? probably. ConnectionDomain*
        // create_connection(const std::string_view host_ip, u16 port);

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
