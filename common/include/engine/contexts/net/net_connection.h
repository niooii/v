//
// Created by niooi on 7/31/2025.
//

#pragma once

#include <defs.h>
#include <entt/entt.hpp>
#include <string>

extern "C" {
#include <enet.h>
}

namespace v {
    class NetworkContext; // forward declaration
    
    template <typename Derived>
    class NetChannel;
    
    template <typename T>
    concept DerivedFromChannel = std::is_base_of_v<NetChannel<T>, T>;

    class NetConnection {
        friend class NetworkContext;

    public:
        /// Factory method for NetworkContext to create connections
        static std::unique_ptr<NetConnection> create(NetworkContext* ctx, const std::string& host, u16 port)
        {
            return std::unique_ptr<NetConnection>(new NetConnection(ctx, host, port));
        }
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

        ~NetConnection();

    private:
        // TODO! change this to accept a Option<HostOptions> struct, if not provided then
        // it will not bind the host to any port/addr. (client usage, vs server usage
        // where you will pass in a host struct)
        NetConnection(class NetworkContext* ctx, const std::string& host, u16 port);

        ENetHost* enet_host_;

        const std::string host_;
        const u16         port_;
        entt::entity      entity_;

        // Pointer guarenteed to be alive here
        NetworkContext* net_ctx_;
    };
} // namespace v
