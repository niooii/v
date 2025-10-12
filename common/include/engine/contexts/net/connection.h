//
// Created by niooi on 7/31/2025.
//

#pragma once

#include <atomic>
#include <defs.h>
#include <engine/contexts/net/ctx.h>
#include <engine/contexts/net/connection_fsm.h>
#include <engine/contexts/net/channel_fsm.h>
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
#include <memory>
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
        friend struct Active;
        friend struct Disconnecting;

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

                // Check if remote already created this channel
                auto id_it = recv_c_ids_.find(std::string(T::unique_name()));
                if (id_it != recv_c_ids_.end())
                {
                    // Remote channel exists, transition from RemoteOnly -> Linked
                    u32 remote_uid = id_it->second;
                    auto fsm_it = channel_fsms_.find(remote_uid);
                    if (fsm_it != channel_fsms_.end() && fsm_it->second)
                    {
                        LOG_DEBUG("Channel {} found remotely, linking", std::string(T::unique_name()));
                        fsm_it->second->context.local_channel = channel;
                        fsm_it->second->fsm.immediateChangeTo<Linked>();
                    }
                }
                else
                {
                    // Create LocalOnly FSM - waiting for remote
                    auto wrapper = std::make_unique<ChannelFSMWrapper>(
                        std::string(T::unique_name()), 0, static_cast<NetChannelBase*>(channel));
                    // FSM starts in LocalOnly state by default (first state)

                    // Store in local map keyed by name until remote connects
                    local_channel_fsms_[std::string(T::unique_name())] = std::move(wrapper);
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

            if (!fsm_.isActive<Active>())
            {
                LOG_ERROR("[conn={}] CHANNEL| packet queued (connection not Active yet)", (void*)this);
                // allocate outgoing queue on demand
                if (!fsm_context_.outgoing_packets)
                {
                    fsm_context_.outgoing_packets = new moodycamel::ConcurrentQueue<ENetPacket*>();
                }

                fsm_context_.outgoing_packets->enqueue(packet);
            }
            else
            {
                LOG_ERROR("[conn={}] CHANNEL| packet sent immediately", (void*)this);
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

        // Connection state machine
        ConnectionContext fsm_context_;
        ConnectionFSMRoot::Instance fsm_;

        // Channel FSM wrapper - stores context and FSM together
        struct ChannelFSMWrapper {
            ChannelContext context;
            ChannelFSMRoot::Instance fsm;

            ChannelFSMWrapper() : fsm(context) {}
            ChannelFSMWrapper(std::string name, u32 remote_uid, NetChannelBase* local_channel)
                : context{std::move(name), remote_uid, local_channel, nullptr}, fsm(context) {}
        };

        // Channel state machines keyed by remote UID (for RemoteOnly and Linked states)
        ud_map<u32, std::unique_ptr<ChannelFSMWrapper>> channel_fsms_{};

        // LocalOnly channel FSMs keyed by channel name (before remote connection)
        ud_map<std::string, std::unique_ptr<ChannelFSMWrapper>> local_channel_fsms_{};

        // Map channel name to remote UID for lookup
        ud_map<std::string, u32> recv_c_ids_{};

        // Queue for packets arriving before CHANNEL| handshake completes
        // Keyed by remote channel ID
        ud_map<u32, std::unique_ptr<moodycamel::ConcurrentQueue<ENetPacket*>>> pending_channel_packets_{};

        // a mutex for all the maps above
        // TODO! wrap the maps in this for raii stuff
        RwLock<int> map_lock_;

        // instances of channels mapped to their string name. will only be accessed
        // from the main thread
        ud_map<std::string_view, NetChannelBase*> c_insts_{};

        moodycamel::ConcurrentQueue<ENetPacket*> packet_destroy_queue_{};

        // Pointer guarenteed to be alive here
        NetworkContext* net_ctx_;

        // For convenience, set by the creator of the net connections.
        // TODO! holy lifetime graph just dont use shared ptr since
        // we're not doing any sync with this object in particular?? :sob:
        // OR JUST USE STATE MACHINE PLEASE
        std::shared_ptr<NetConnection> shared_con_;
    };
} // namespace v
