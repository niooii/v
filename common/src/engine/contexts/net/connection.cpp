//
// Created by niooi on 8/1/2025.
//

#include <cstring>
#include <defs.h>
#include <enet.h>
#include <engine/contexts/net/channel.h>
#include <engine/contexts/net/connection.h>
#include <engine/contexts/net/ctx.h>
#include <stdexcept>
#include "moodycamel/concurrentqueue.h"

namespace v {
    // create an outgoing connection
    NetConnection::NetConnection(NetworkContext* ctx, const std::string& host, u16 port) :
        net_ctx_(ctx), conn_type_(ConnectionType::Outgoing), Domain(ctx->engine_)
    {
        entity_ = ctx->engine_.registry().create();

        ENetAddress address;
        if (enet_address_set_host(&address, host.c_str()) != 0)
        {
            enet_deinitialize();
            throw std::runtime_error("Failed to set host address");
        }
        address.port = port;

        peer_ = enet_host_connect(
            *ctx->outgoing_host_.write(), &address, 4 /* 4 channels */, 0);

        if (!peer_)
        {
            LOG_ERROR("Failed to connect to peer at {}:{}", host, port);
            throw std::runtime_error("bleh");
        }

        peer_->data = (void*)this;
    }

    // just an incoming connection, its whatever
    NetConnection::NetConnection(NetworkContext* ctx, ENetPeer* peer) :
        net_ctx_(ctx), peer_(peer), conn_type_(ConnectionType::Incoming),
        Domain(ctx->engine_)
    {
        peer_->data = (void*)this;
    }

    // at this point there should be no more references internally
    NetConnection::~NetConnection()
    {
        // internally queues disconnect
        if (!remote_disconnected_)
        {
            // make it known that the connection was disconnected locally
            peer_->data = NULL;

            enet_peer_disconnect(peer_, 0);
        }
    }

    template <DerivedFromChannel T>
    NetChannel<T, typename T::PayloadT>& NetConnection::create_channel()
    {
        if (auto comp = engine_.registry().try_get<T*>(entity_))
        {
            LOG_WARN("Channel {} not created, as it already exists...", T::unique_name());
            return static_cast<NetChannel<T, typename T::PayloadT>&>(**comp);
        }

        std::shared_ptr<NetConnection> shared_this;
        {
            auto connections = net_ctx_->connections_.read();
            auto it          = connections->find(peer_);
            if (it != connections->end())
            {
                shared_this = it->second;
            }
        }

        auto channel = engine_.registry().emplace<T*>(entity_, new T());
        channel->set_connection(shared_this);

        {
            // acquire all the mapping info locks (TODO! maybe clump them up?) so
            // the network io thread must halt as we're checking to see if we can
            // fill in the information right now

            auto lock = map_lock_.write();

            c_insts_[std::string(T::unique_name())] = channel;

            auto id_it = recv_c_ids_.find(std::string(T::unique_name()));
            if (id_it != recv_c_ids_.end())
            {
                u32   id     = id_it->second;
                auto& info   = (recv_c_info_)[id];
                info.channel = channel;
                // drain the precreation queue into the incoming queue
                ENetPacket* packet;
                while (info.before_creation_packets->try_dequeue(packet))
                {
                    channel->take_packet(packet);
                }

                delete info.before_creation_packets;
            }
        }

        LOG_TRACE("Local channel created");

        // send over creation data and our runtime uid
        std::string message =
            std::format("CHANNEL|{}|{}", T::unique_name(), runtime_type_id<T>());

        ENetPacket* packet = enet_packet_create(
            message.c_str(),
            message.length() + 1, // including null terminator
            ENET_PACKET_FLAG_RELIABLE);

        enet_peer_send(peer_, 0, packet); // Use channel 0 for control messages

        LOG_TRACE("Queued channel creation packet send");

        return static_cast<NetChannel<T, typename T::PayloadT>&>(*channel);
    }

    void NetConnection::request_close()
    {
        net_ctx_->cleanup_tracking(peer_);

        auto lock = map_lock_.write();
        // delete all channel pointers
        for (auto& [a, b] : c_insts_)
        {
            delete b;
        }
    }

    void NetConnection::update()
    {
        // update all channels (runs the callbacks for recieved/parsed data)
        for (auto& [name, chnl] : c_insts_)
        {
            chnl->update();
        }

        // destroys the consumed packets
        ENetPacket* packet;
        while (packet_destroy_queue_.try_dequeue(packet))
        {
            enet_packet_destroy(packet);
        }
    }


    void NetConnection::handle_raw_packet(ENetPacket* packet)
    {
        // TODO! auto channel creation, routing, etc
        LOG_TRACE("Got packet");
        // check if its a channel creation request
        constexpr std::string_view prefix = "CHANNEL|";
        if (UNLIKELY(
                packet->dataLength > prefix.size() &&
                memcmp(packet->data, prefix.data(), prefix.size()) == 0))
        {
            LOG_TRACE(
                "Packet is channel creation request: {}",
                std::string(
                    reinterpret_cast<const char*>(packet->data), packet->dataLength));

            const char* data =
                reinterpret_cast<const char*>(packet->data) + prefix.size();


            size_t remaining_len = packet->dataLength - prefix.size();

            std::string message(data, remaining_len);

            size_t separator = message.find('|');
            if (separator != std::string::npos)
            {
                std::string channel_name = message.substr(0, separator);
                std::string channel_id   = message.substr(separator + 1);

                u32 c_id;
                auto [ptr, ec] = std::from_chars(
                    channel_id.data(), channel_id.data() + channel_id.size(), c_id);

                if (ec != std::errc())
                {
                    LOG_ERROR("Invalid channel ID: {}", channel_id);
                    return;
                }

                // populate info maps

                auto lock = map_lock_.write();

                recv_c_ids_[channel_name] = c_id;
                auto [it, inserted]       = recv_c_info_.try_emplace(c_id);
                auto& info                = it->second;
                info.name                 = std::move(channel_name);

                auto inst = c_insts_.find(channel_name);
                if (inst != c_insts_.end())
                {
                    // the channel instance already exists locally
                    info.channel = inst->second;
                }
                else
                {
                    info.before_creation_packets =
                        new moodycamel::ConcurrentQueue<ENetPacket*>();
                }
                LOG_TRACE("Channel maps populated");
            }
            else
            {
                LOG_WARN(
                    "Bad channel creation packet",
                    std::string(
                        reinterpret_cast<const char*>(packet->data), packet->dataLength));
            }
            return;
        }

        // else its a regular packet traveling to a channel
        if (packet->dataLength < sizeof(u32))
        {
            LOG_WARN("Packet too small to contain channel ID, dropping");
            enet_packet_destroy(packet);
            return;
        }

        // extract channel ID from first 4 bytes
        u32 channel_id;
        // TODO! perserve endianness
        std::memcpy(&channel_id, packet->data, sizeof(u32));

        {
            auto lock = map_lock_.read();
            auto it   = recv_c_info_.find(channel_id);
            if (it == recv_c_info_.end())
            {
                LOG_WARN("Invalid packet, no such channel id exists: {}", channel_id);
                enet_packet_destroy(packet);
                return;
            }

            auto& info = it->second;
            if (!info.channel)
            {
                info.before_creation_packets->enqueue(packet);
            }
            else
            {
                info.channel->take_packet(packet);
            }
        }
    }
} // namespace v
