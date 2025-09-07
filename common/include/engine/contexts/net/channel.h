//
// Created by niooi on 7/31/2025.
//

#pragma once

#include <defs.h>
#include <engine/contexts/net/connection.h>
#include <memory>
#include "engine/contexts/net/listener.h"
#include "moodycamel/concurrentqueue.h"

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

    class NetChannelBase {
        friend NetConnection;
    public:
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

        ~NetChannel()
        {
            // give packets back to owning connection for destruction

            std::unique_ptr<std::tuple<Payload, ENetPacket*>> elem;

            while (incoming_.try_dequeue(elem))
            {
                conn_->packet_destroy_queue_.enqueue(std::get<ENetPacket*>(*elem));
            }
        }

        NetChannelComponent<Derived, Payload>& create_component(entt::entity id)
        {
            Engine& engine = conn_->net_ctx_->engine_;

            auto& component =
                engine.registry().emplace<NetChannelComponent<Derived, Payload>>(id);
            return component;
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

        void send(char* buf, u64 len) override;

    protected:
        /// A function to construct a parsed type from bytes. The bytes* ptr will stay
        /// alive until after the payload is consumed by listener(s). Any implementation
        /// of this function should be thread safe or isolated, as it runs on the
        /// networking io thread.
        /// @note Default implementation is for Bytes (std::span<const char*>), for other
        /// payload types, the derived class must manually implement this method.
        static Payload parse(const u8* bytes, u64 len)
        {
            if constexpr (std::is_same_v<Payload, Bytes>)
            {
                return std::span<const char>(reinterpret_cast<const char*>(bytes), len);
            }
            else
            {
                // require Derived::parse() to exist
                if constexpr (requires { Derived::parse(); })
                {
                    return Derived::parse(bytes, len);
                }
                else
                {
                    static_assert(
                        !std::is_same_v<Payload, Payload>,
                        "No parser for Payload: provide `static Payload "
                        "parse(const u8* bytes, u64 len)` in Derived class");
                }
            }
        }

    private:
        NetChannel(std::shared_ptr<NetConnection> conn) : conn_(conn) {}

        /// Calls the listeners and hands packets back to the NetConnection.
        void update() override
        {
            // call all the components
            // i could avoid the nested loop but if i batch them users have to concern
            // themselves with consistency so its whatever

            std::unique_ptr<std::tuple<Payload, ENetPacket*>> elem;

            Engine& engine = conn_->net_ctx_->engine_;

            auto view =
                engine.registry().template view<NetChannelComponent<Derived, Payload>>();

            while (incoming_.try_dequeue(elem))
            {
                for (auto& [entity, comp] : view.each())
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
            Payload p = parse(packet->data, packet->dataLength);
            std::unique_ptr<std::tuple<Payload, ENetPacket*>> elem =
                std::make_unique(p, packet);

            incoming_.enqueue(std::move(elem));
        }

        moodycamel::ConcurrentQueue<std::unique_ptr<std::tuple<Payload, ENetPacket*>>>
            incoming_{};

        // pointer to the owning netconnection, which is guarenteed to be valid
        // shared ptr so the owning connection can only destroy itself when all
        // channels are gone (will be destroyed when requesting a connection close)
        std::shared_ptr<NetConnection> conn_;
    };

    /// A channel to make sure the program builds correctly
    class TestChannel : NetChannel<TestChannel> {};
} // namespace v
