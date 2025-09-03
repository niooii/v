//
// Created by niooi on 7/31/2025.
//

#pragma once

#include <defs.h>
#include <engine/contexts/net/ctx.h>
#include <entt/entt.hpp>
#include <string>

extern "C" {
#include <enet.h>
}

namespace v {
    struct HostOptions {};

    template <typename Derived>
    class NetChannel;

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

        /// Request a close to this connection.
        /// Other functions can no longer access this connection, but
        /// the connection will remain alive until the last instance of it's handle
        /// is gone.
        void request_close() { net_ctx_->req_close(peer_); }

        // FORCEINLINE const std::string& host() { return host_; }

        // FORCEINLINE const u16 port() { return port_; }

        FORCEINLINE const ENetPeer* peer() { return peer_; }

        ~NetConnection();

    private:
        /// The constructor for an outgoing connection.
        NetConnection(class NetworkContext* ctx, const std::string& host, u16 port);
        /// The constructor for an incoming connection.
        NetConnection(NetworkContext* ctx, ENetPeer* peer);

        /// Handles an incoming raw packet
        void handle_raw_packet(ENetPacket* packet);

        entt::entity entity_;
        ENetPeer*    peer_;

        // whether the connection was disconnected on our side or not.
        // if its not, we have to handle removing it from maps and internal tracking,
        // if it WAS disconnected by us, its already gone.
        bool remote_disconnected_{};

        // Pointer guarenteed to be alive here
        NetworkContext* net_ctx_;
    };
} // namespace v
