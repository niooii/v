//
// Created by niooi on 7/31/2025.
//

#pragma once

#include <atomic>
#include <defs.h>
#include <engine/contexts/net/ctx.h>
#include <entt/entt.hpp>
#include <string>
#include "engine/engine.h"
#include "moodycamel/concurrentqueue.h"
#include "unordered_dense.h"

extern "C" {
#include <enet.h>
}

namespace v {
    template <typename Derived, typename Payload>
    class NetChannel;

    template <typename T>
    concept HasPayloadAlias = requires { typename T::PayloadT; };

    template <typename T>
    concept DerivedFromChannel =
        HasPayloadAlias<T> && std::is_base_of_v<NetChannel<T, typename T::PayloadT>, T>;

    class NetConnection : Domain {
        friend class NetworkContext;

        template <typename T, typename P>
        friend class NetChannel;

    public:
        /// Creates a NetChannel for isolated network communication.
        /// If it already exists, it throws a warning and returns the one existing.
        /// Channels created with this method may be queried from the engine's main
        /// registry via it's type pointer, e.g. registry.view<SomeChannel*>(); Note:
        /// someone on the other side of the connection must also explicitly create this
        /// channel. Until then, messages are sent but accumulated on the reciever's side
        /// in a queue.
        template <DerivedFromChannel T>
        NetChannel<T, typename T::PayloadT>& create_channel();

        /// Returns a NetChannel or nullptr if it doesn't exist.
        template <DerivedFromChannel T>
        FORCEINLINE NetChannel<T, typename T::PayloadT>* get_channel()
        {
            if (auto channel = net_ctx_->engine_.registry().try_get<T*>(entity_))
            {
                return channel;
            }

            return nullptr;
        }

        /// Request a close to this connection.
        /// Other functions can no longer access this connection, but
        /// the connection will remain alive until the last instance of it's handle
        /// is gone.
        void request_close();

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

        ENetPeer* peer_;

        ConnectionType conn_type_;

        // whether the connection was disconnected on our side or not.
        // if its not, we have to handle removing it from maps and internal tracking,
        // if it WAS disconnected by us, its already gone.
        std::atomic_bool remote_disconnected_{};

        struct NetChannelInfo {
            std::string           name;
            class NetChannelBase* channel{ nullptr };
            // we queue the packets recieved before initialization
            moodycamel::ConcurrentQueue<ENetPacket*>* before_creation_packets{ nullptr };
        };

        ankerl::unordered_dense::map<u32, NetChannelInfo>               recv_c_info_{};
        ankerl::unordered_dense::map<std::string, u32>                  recv_c_ids_{};
        ankerl::unordered_dense::map<std::string_view, NetChannelBase*> c_insts_{};

        // a mutex for all the maps above
        RwLock<int> map_lock_;

        /// The main update function called synchronously. 
        void update();

        moodycamel::ConcurrentQueue<ENetPacket*> packet_destroy_queue_{};

        // Pointer guarenteed to be alive here
        NetworkContext* net_ctx_;
    };
} // namespace v
