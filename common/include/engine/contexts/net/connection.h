//
// Created by niooi on 7/31/2025.
//

#pragma once

#include <atomic>
#include <defs.h>
#include <engine/contexts/net/ctx.h>
#include <entt/entt.hpp>
#include <format>
#include <string>
#include "engine/engine.h"

// Forward declaration for runtime_type_id function
namespace v {
    template <typename T>
    inline uint32_t runtime_type_id();
}
#include <containers/ud_map.h>
#include "moodycamel/concurrentqueue.h"

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


    enum class NetConnectionResult { TimedOut = 0, ConnWaiting, Success };

    class NetConnection : public Domain<NetConnection> {
        friend class NetworkContext;
        friend struct NetworkEvent;

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
        NetChannel<T, typename T::PayloadT>& create_channel()
        {
            if (!engine_.is_valid_entity(entity_))
            {
                LOG_WARN("Connection is dead, not creating channel..");
                // because entity is destroyed in handling DestroyConnection, which should
                // process all the queued events with this connection as well, and destroy
                // this object.
                LOG_ERROR("You should not be on this branch, something is wrong..");
                assert(false);
            }

            if (auto comp = engine_.try_get_component<T*>(entity_))
            {
                LOG_WARN(
                    "Channel {} not created, as it already exists...", T::unique_name());
                return static_cast<NetChannel<T, typename T::PayloadT>&>(**comp);
            }

            auto& channel = engine_.add_component<T*>(entity_, new T());
            channel->init(shared_con_);

            {
                // acquire all the mapping info locks so
                // the network io thread must halt as we're checking to see if we can
                // fill in the information right now

                auto lock = map_lock_.write();

                c_insts_[T::unique_name()] = channel;

                auto id_it = recv_c_ids_.find(std::string(T::unique_name()));
                if (id_it != recv_c_ids_.end())
                {
                    u32   id     = id_it->second;
                    auto& info   = (recv_c_info_)[id];
                    info.channel = channel;
                    // drain the precreation queue into the incoming queue
                    info.drain_queue(channel);
                }
            }

            // send over creation data and our runtime uid
            // type_name_dbg<T>();
            LOG_TRACE("Local channel created with unique name {}", T::unique_name());
            std::string message =
                std::format("CHANNEL|{}|{}", T::unique_name(), runtime_type_id<T>());

            ENetPacket* packet = enet_packet_create(
                message.c_str(),
                message.length() + 1, // including null terminator
                ENET_PACKET_FLAG_RELIABLE);

            if (pending_activation_)
            {
                // allocate outgoing queue on demand
                if (!outgoing_packets_)
                {
                    outgoing_packets_ = new moodycamel::ConcurrentQueue<ENetPacket*>();
                }

                outgoing_packets_->enqueue(packet);
            }
            else
            {
                enet_peer_send(peer_, 0, packet); // use channel 0 for control messages
            }

            LOG_TRACE("Queued channel creation packet send");

            return static_cast<NetChannel<T, typename T::PayloadT>&>(*channel);
        }

        /// Returns a NetChannel or nullptr if it doesn't exist.
        template <DerivedFromChannel T>
        FORCEINLINE NetChannel<T, typename T::PayloadT>* get_channel()
        {
            if (auto channel = net_ctx_->engine_.try_get_component<T*>(entity_))
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

        /// Activate the connection (called from main thread)
        void activate_connection();

        // FORCEINLINE const std::string& host() { return host_; }

        // FORCEINLINE const u16 port() { return port_; }

        FORCEINLINE const ENetPeer* peer() { return peer_; }

        ~NetConnection();

    private:
        /// The constructor for an outgoing connection.
        NetConnection(
            class NetworkContext* ctx, const std::string& host, u16 port,
            f64 connection_timeout);
        /// The constructor for an incoming connection.
        NetConnection(NetworkContext* ctx, ENetPeer* peer);

        /// Handles an incoming raw packet
        void handle_raw_packet(ENetPacket* packet);

        /// The main update function called synchronously
        NetConnectionResult update();

        ENetPeer* peer_;

        ConnectionType conn_type_;

        // whether the connection was disconnected on our side or not.
        // if its not, we have to handle removing it from maps and internal tracking,
        // if it WAS disconnected by us, its already gone.
        std::atomic_bool remote_disconnected_{};

        // whether the connection is pending activation from main thread
        std::atomic_bool pending_activation_{ true };

        // packets received while pending activation
        moodycamel::ConcurrentQueue<ENetPacket*> pending_packets_{};

        struct NetChannelInfo {
            ~NetChannelInfo()
            {
                // yea no leaks today pls
                if (before_creation_packets)
                    delete before_creation_packets;
            }

            void drain_queue(class NetChannelBase* channel);

            std::string     name;
            NetChannelBase* channel{ nullptr };
            // we queue the packets recieved before initialization
            moodycamel::ConcurrentQueue<ENetPacket*>* before_creation_packets{ nullptr };
        };

        // maps for tracking channel stuff
        ud_map<u32, NetChannelInfo> recv_c_info_{};
        ud_map<std::string, u32>    recv_c_ids_{};

        // a mutex for all the maps above
        // TODO! wrap the maps in this for raii stuff
        RwLock<int> map_lock_;

        // instances of channels mapped to their string name. will only be accessed
        // from the main thread
        ud_map<std::string_view, NetChannelBase*> c_insts_{};

        moodycamel::ConcurrentQueue<ENetPacket*> packet_destroy_queue_{};

        // outgoing packet queue for packets sent before connection is ready
        moodycamel::ConcurrentQueue<ENetPacket*>* outgoing_packets_{ nullptr };

        /// The amount of elapsed time since we requested the connection (since
        /// construction)
        Stopwatch since_open_{};

        /// Amount of time to wait for the connection to succeed until we nuke the
        /// connection
        f64 connection_timeout_;

        // Pointer guarenteed to be alive here
        NetworkContext* net_ctx_;

        // For convenience, set by the creator of the net connections.
        // TODO! holy lifetime graph just dont use shared ptr since
        // we're not doing any sync with this object in particular?? :sob:
        // OR JUST USE STATE MACHINE PLEASE
        std::shared_ptr<NetConnection> shared_con_;
    };
} // namespace v
