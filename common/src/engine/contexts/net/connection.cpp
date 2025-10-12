//
// Created by niooi on 8/1/2025.
//

#include <cstring>
#include <defs.h>
#include <enet.h>
#include <engine/contexts/net/channel.h>
#include <engine/contexts/net/connection.h>
#include <engine/contexts/net/ctx.h>
#include <memory>
#include <stdexcept>
#include "moodycamel/concurrentqueue.h"

namespace v {
    // create an outgoing connection
    NetConnection::NetConnection(
        NetworkContext* ctx, const std::string& host, u16 port, f64 connection_timeout) :
        net_ctx_(ctx), conn_type_(ConnectionType::Outgoing), Domain(ctx->engine_),
        fsm_context_{this, ctx, nullptr, connection_timeout},
        fsm_{fsm_context_}
    {
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
        fsm_context_.peer = peer_;

        // Outgoing connections start in Connecting state by default
        // (first state in FSM definition)

        LOG_TRACE("Outgoing connection initialized");
    }

    // just an incoming connection, its whatever
    NetConnection::NetConnection(NetworkContext* ctx, ENetPeer* peer) :
        net_ctx_(ctx), peer_(peer), conn_type_(ConnectionType::Incoming),
        Domain(ctx->engine_),
        fsm_context_{this, ctx, peer, 0.0},
        fsm_{fsm_context_}
    {
        peer_->data = (void*)this;

        // Incoming connections are already connected via ENet, immediately transition to Active
        fsm_.immediateChangeTo<Active>();

        LOG_TRACE("Incoming connection initialized");
    }

    // at this point there should be no more references internally
    NetConnection::~NetConnection()
    {
        // internally queues disconnect
        // if remote_disconnected_, then the peer may no longer be a valid pointer
        if (!fsm_context_.remote_disconnected)
        {
            // enqueue onto the IO thread no race condition
            net_ctx_->enqueue_io([this] { enet_peer_disconnect(peer_, 0); });
        }

        // clear the peer data pointer - safe to do here since destructor runs on main
        // thread after all lifecycle events have been processed
        if (peer_->data == (void*)this)
            peer_->data = NULL;

        ENetPacket* packet;
        while (fsm_context_.pending_packets.try_dequeue(packet))
            enet_packet_destroy(packet);

        // cleanup outgoing packets if they weren't sent
        if (fsm_context_.outgoing_packets)
        {
            while (fsm_context_.outgoing_packets->try_dequeue(packet))
                enet_packet_destroy(packet);
            delete fsm_context_.outgoing_packets;
        }

        LOG_TRACE("Connection destroyed");
    }


    void NetConnection::request_close()
    {
        fsm_.changeTo<Disconnecting>();
    }

    void NetConnection::activate_connection()
    {
        fsm_.changeTo<Active>();
    }

    NetConnectionResult NetConnection::update()
    {
        // Update the FSM, which will call the appropriate state's update()
        fsm_.update();

        // Check what state we're in to return appropriate result
        if (fsm_.isActive<Connecting>())
        {
            return NetConnectionResult::ConnWaiting;
        }
        else if (fsm_.isActive<Disconnecting>())
        {
            return NetConnectionResult::TimedOut;
        }

        return NetConnectionResult::Success;
    }


    void NetConnection::handle_raw_packet(ENetPacket* packet)
    {
        // If connection is not yet active, queue the packet
        if (!fsm_.isActive<Active>())
        {
            fsm_context_.pending_packets.enqueue(packet);
            return;
        }

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

                // Handle channel creation using FSM
                {
                    auto lock = map_lock_.write();

                    LOG_ERROR("[conn={}] CHANNEL| handler: channel_name={}, c_id={}", (void*)this, channel_name, c_id);

                    recv_c_ids_[channel_name] = c_id;

                    // Check if local channel already exists
                    auto local_it = c_insts_.find(channel_name);
                    LOG_ERROR("Local channel lookup: {}", local_it != c_insts_.end() ? "FOUND" : "NOT FOUND");
                    if (local_it != c_insts_.end())
                    {
                        // Local exists, transition from LocalOnly -> Linked
                        // Move FSM from local_channel_fsms_ to channel_fsms_
                        auto local_fsm_it = local_channel_fsms_.find(channel_name);
                        LOG_ERROR("LocalOnly FSM lookup: {}", local_fsm_it != local_channel_fsms_.end() ? "FOUND" : "NOT FOUND");
                        if (local_fsm_it != local_channel_fsms_.end())
                        {
                            // Transition existing LocalOnly FSM to Linked
                            local_fsm_it->second->context.remote_uid = c_id;
                            local_fsm_it->second->fsm.immediateChangeTo<Linked>();

                            // Move to channel_fsms_ with remote UID as key
                            channel_fsms_[c_id] = std::move(local_fsm_it->second);
                            local_channel_fsms_.erase(local_fsm_it);

                            // Drain any queued packets for this channel
                            auto pending_it = pending_channel_packets_.find(c_id);
                            LOG_ERROR("Checking pending_channel_packets_[{}]: {}", c_id,
                                (pending_it != pending_channel_packets_.end() && pending_it->second) ? "EXISTS" : "DOES NOT EXIST");
                            if (pending_it != pending_channel_packets_.end() && pending_it->second)
                            {
                                int count = 0;
                                ENetPacket* queued_packet;
                                while (pending_it->second->try_dequeue(queued_packet))
                                {
                                    channel_fsms_[c_id]->context.local_channel->take_packet(queued_packet);
                                    count++;
                                }
                                pending_channel_packets_.erase(pending_it);
                                LOG_ERROR("Drained {} pending packets for channel {}", count, channel_name);
                            }

                            LOG_DEBUG("Channel {} linked (local found first)", channel_name);
                        }
                        else
                        {
                            LOG_WARN("Channel {} local instance exists but FSM not found", channel_name);
                        }
                    }
                    else
                    {
                        // Remote created first, start in RemoteOnly state
                        auto wrapper = std::make_unique<ChannelFSMWrapper>(channel_name, c_id, nullptr);
                        wrapper->fsm.immediateChangeTo<RemoteOnly>();
                        channel_fsms_[c_id] = std::move(wrapper);

                        // Move any queued packets to the FSM's queue
                        auto pending_it = pending_channel_packets_.find(c_id);
                        if (pending_it != pending_channel_packets_.end() && pending_it->second)
                        {
                            ENetPacket* queued_packet;
                            while (pending_it->second->try_dequeue(queued_packet))
                            {
                                if (!channel_fsms_[c_id]->context.packet_queue)
                                {
                                    channel_fsms_[c_id]->context.packet_queue =
                                        std::make_unique<moodycamel::ConcurrentQueue<ENetPacket*>>();
                                }
                                channel_fsms_[c_id]->context.packet_queue->enqueue(queued_packet);
                            }
                            pending_channel_packets_.erase(pending_it);
                        }

                        LOG_DEBUG("Channel {} in RemoteOnly (waiting for local)", channel_name);
                    }
                }

                LOG_TRACE("Channel {} linked to remote uid {}", channel_name, c_id);
            }
            else
            {
                LOG_WARN(
                    "Bad channel creation packet",
                    std::string(
                        reinterpret_cast<const char*>(packet->data), packet->dataLength));
            }

            // Channel creation packets are not forwarded to channels, so destroy them here
            enet_packet_destroy(packet);
            return;
        }

        // else its a regular packet traveling to a channel
        if (packet->dataLength < sizeof(u32))
        {
            LOG_WARN("Packet too small to contain channel ID, dropping");
            enet_packet_destroy(packet);
            return;
        }

        // extract channel ID from first 4 bytes, converting from network byte order
        u32 channel_id;
        std::memcpy(&channel_id, packet->data, sizeof(u32));
        channel_id = ntohl(channel_id);

        LOG_ERROR("[conn={}] Received data packet with channel_id={}", (void*)this, channel_id);

        {
            auto lock = map_lock_.write();
            auto fsm_it = channel_fsms_.find(channel_id);
            LOG_ERROR("FSM lookup result: {}", fsm_it != channel_fsms_.end() ? "FOUND" : "NOT FOUND");
            if (fsm_it == channel_fsms_.end())
            {
                // Queue packet until CHANNEL handshake completes
                if (!pending_channel_packets_[channel_id])
                {
                    pending_channel_packets_[channel_id] =
                        std::make_unique<moodycamel::ConcurrentQueue<ENetPacket*>>();
                }
                pending_channel_packets_[channel_id]->enqueue(packet);
                LOG_TRACE("Queued packet for unknown channel_id {}", channel_id);
                return;
            }

            auto& wrapper = fsm_it->second;
            if (wrapper->fsm.isActive<Linked>())
            {
                // Route directly to local channel
                wrapper->context.local_channel->take_packet(packet);
            }
            else if (wrapper->fsm.isActive<RemoteOnly>() || wrapper->fsm.isActive<LocalOnly>())
            {
                // Queue packet until channel is fully linked
                if (!wrapper->context.packet_queue)
                {
                    wrapper->context.packet_queue =
                        std::make_unique<moodycamel::ConcurrentQueue<ENetPacket*>>();
                }
                wrapper->context.packet_queue->enqueue(packet);
                LOG_TRACE("Queued packet for channel {}", wrapper->context.name);
            }
            else
            {
                LOG_WARN("Packet received for channel {} in unexpected state", wrapper->context.name);
                enet_packet_destroy(packet);
            }
        }
    }
} // namespace v
