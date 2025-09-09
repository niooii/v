//
// Created by niooi on 7/31/2025.
//

#pragma once

#include <defs.h>
#include <engine/contexts/net/connection.h>
#include <memory>
#include "engine/contexts/net/listener.h"
#include "moodycamel/concurrentqueue.h"
#include "unordered_dense.h"

namespace v {
    inline std::atomic<u32>& _global_type_counter()
    {
        static std::atomic<u32> c{ 1 };
        return c;
    }

    template <typename T>
    inline uint32_t runtime_type_id()
    {
        static const u32 id = _global_type_counter()++;
        return id;
    }

    typedef std::span<const std::byte> Bytes;

    template <typename Payload>
    using OnRecvCallback = std::function<void(const Payload&)>;

    template <typename Derived, typename Payload>
    struct NetChannelComponent {
        OnRecvCallback<Payload> on_recv;
    };

    struct NetDestructionTracker {
        u8;
    };

    class NetChannelBase {
        friend NetConnection;

    public:
        virtual ~NetChannelBase() = default;
        virtual void send(char* buf, u64 len) = 0;

    protected:
        /// Takes ownership of a packet until consumed.
        virtual void take_packet(ENetPacket* packet) = 0;
        /// Update internal stuff function to be ran on the main thread
        virtual void update() = 0;
    };

    /// An abstract channel class, created and managed by a NetConnection connection
    /// object, meaning a channel is unique to a specific NetConnection only. To create a
    /// channel, inherit from this class.
    /// E.g. class ChatChannel : public NetChannel<ChatChannel> {};
    template <typename Derived, typename Payload = Bytes>
    class NetChannel : NetChannelBase {
        friend NetConnection;

    public:
        using PayloadT = Payload;

        ~NetChannel() override
        {
            // give packets back to owning connection for destruction

            std::unique_ptr<std::tuple<Payload, ENetPacket*>> elem;

            while (incoming_.try_dequeue(elem))
            {
                conn_->packet_destroy_queue_.enqueue(std::get<ENetPacket*>(*elem));
            }

            entt_conn_.release();

            conn_.reset();
        }

        NetChannelComponent<Derived, Payload>& create_component(entt::entity id)
        {
            Engine& engine = conn_->net_ctx_->engine_;

            // TODO! use find buddy
            if (components_.contains(id)) {
                LOG_WARN("Entity already has a NetChannelComponent component");
                return components_.at(id);
            }

            components_[id] = NetChannelComponent<Derived, Payload>{};

            // tag it so we can listen for its destruction
            engine.registry().emplace_or_replace<NetDestructionTracker>(id);

            return components_[id];
        }

        /// Get the owning NetConnection.
        FORCEINLINE const class std::shared_ptr<NetConnection> connection_info()
        {
            return conn_;
        };

        /// Assigns a unique name to the channel, to be used for communication
        /// over a network.
        /// Assigns the name of the struct and it's namespaces
        /// (e.g. "v::Chat::ChatChannel").
        static constexpr const std::string_view unique_name()
        {
            return type_name<Derived>();
        }

        void send(char* buf, u64 len) override
        {
            u32 channel_id = runtime_type_id<Derived>();

            // create packet with u32 prefix + payload data
            const u64   total_len = sizeof(u32) + len;
            ENetPacket* packet =
                enet_packet_create(nullptr, total_len, ENET_PACKET_FLAG_RELIABLE);

            if (!packet)
            {
                LOG_ERROR(
                    "Failed to create packet for channel {}", Derived::unique_name());
                return;
            }

            // write channel ID as first 4 bytes
            std::memcpy(packet->data, &channel_id, sizeof(u32));

            // write payload data after the channel ID
            std::memcpy(packet->data + sizeof(u32), buf, len);

            // send the packet
            if (conn_->pending_activation_)
            {
                LOG_WARN("Connection is not yet open, queueing packet send");
                
                // allocate outgoing queue on demand
                if (!conn_->outgoing_packets_) {
                    conn_->outgoing_packets_ = new moodycamel::ConcurrentQueue<ENetPacket*>();
                }
                
                conn_->outgoing_packets_->enqueue(packet);
                return; // don't destroy packet, it will be sent later
            }
            
            if (enet_peer_send(conn_->peer_, 0, packet) != 0)
            {
                LOG_ERROR("Failed to send packet on channel {}", Derived::unique_name());
                enet_packet_destroy(packet);
            }
        }

    protected:
        /// A function to construct a parsed type from bytes. The bytes* ptr will stay
        /// alive until after the payload is consumed by listener(s). Any
        /// implementation of this function should be thread safe or isolated, as it
        /// runs on the networking io thread.
        /// @note Default implementation is for Bytes (std::span<const char*>), for
        /// other payload types, the derived class must manually implement this
        /// method.
        static Payload parse(const u8* bytes, u64 len)
        {
            if constexpr (std::is_same_v<Payload, Bytes>)
            {
                return std::span<const char>(reinterpret_cast<const char*>(bytes), len);
            }
            else
            {
                // Call the derived class's parse method
                return Derived::parse(bytes, len);
            }
        }

    protected:
        NetChannel() = default;

    private:

        // explicit init beacuse i dont want derived class to have to explicitly
        // define constructors
        void init(std::shared_ptr<NetConnection> c)
        {
            conn_ = c;
            entt_conn_ = conn_->engine_.registry()
                .on_destroy<NetDestructionTracker>()
                .connect<&NetChannel<Derived, Payload>::cleanup_component_on_entity_destroy>(this);
            ;
        }
        std::shared_ptr<NetConnection> connection() const { return conn_; }

        /// Calls the listeners and hands packets back to the NetConnection.
        void update() override
        {
            // call all the components
            // i could avoid the nested loop but if i batch them users have to concern
            // themselves with consistency so its whatever

            std::unique_ptr<std::tuple<Payload, ENetPacket*>> elem;

            Engine& engine = conn_->net_ctx_->engine_;

            while (incoming_.try_dequeue(elem))
            {
                for (auto& [entity, comp] : components_)
                {
                    if (comp.on_recv)
                        comp.on_recv(std::get<Payload>(*elem));
                }

                // hand packet back to owning connection for destruction
                conn_->packet_destroy_queue_.enqueue(std::get<ENetPacket*>(*elem));
            }
        }

        void take_packet(ENetPacket* packet) override
        {
            // exclude the channel id header
            // TODO! or just use some enet property? idk
            Payload p =
                parse(packet->data + sizeof(u32), packet->dataLength - sizeof(u32));
            std::unique_ptr<std::tuple<Payload, ENetPacket*>> elem =
                std::make_unique<std::tuple<Payload, ENetPacket*>>(p, packet);

            incoming_.enqueue(std::move(elem));
        }

        moodycamel::ConcurrentQueue<std::unique_ptr<std::tuple<Payload, ENetPacket*>>>
            incoming_{};

        // pointer to the owning netconnection, which is guarenteed to be valid
        // shared ptr so the owning connection can only destroy itself when all
        // channels are gone (will be destroyed when requesting a connection close)
        std::shared_ptr<NetConnection> conn_;

        entt::connection entt_conn_;

        // don't use the registry unfortunately, theres no way to make these components
        // unique to each connection, so we keep the public API but implement it
        // differently. for this reason we cannot query netchannelcomponents through the
        // main registry
        ankerl::unordered_dense::map<entt::entity, NetChannelComponent<Derived, Payload>>
            components_{};

        // called when any component of NetDestructionTracker is destroyed
        void cleanup_component_on_entity_destroy(entt::registry& r, entt::entity e) {
            if (components_.contains(e)) {
                LOG_TRACE("Cleaning up channel component for destroyed entity");
                components_.erase(e);
            }
        };
    };

    /// A channel to make sure the program builds correctly
    class TestChannel : NetChannel<TestChannel> {};
} // namespace v
