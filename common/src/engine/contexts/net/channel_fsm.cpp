//
// Created by niooi on 10/11/2025.
//

#include <engine/contexts/net/channel_fsm.h>
#include <engine/contexts/net/channel.h>

namespace v {
    void LocalOnly::enter(ChannelFSMRoot::Control& control) {
        auto& ctx = control.context();

        // Allocate packet queue for packets arriving before remote connects
        if (!ctx.packet_queue) {
            ctx.packet_queue = std::make_unique<moodycamel::ConcurrentQueue<ENetPacket*>>();
        }

        LOG_TRACE("Channel {} entered LocalOnly state (waiting for remote)", ctx.name);
    }

    void LocalOnly::handle_packet(ENetPacket* packet, ChannelFSMRoot::Control& control) {
        auto& ctx = control.context();

        // Queue packet for later processing when remote connects
        if (ctx.packet_queue) {
            ctx.packet_queue->enqueue(packet);
            LOG_TRACE("Channel {} queued packet (LocalOnly)", ctx.name);
        } else {
            // This shouldn't happen, but handle gracefully
            LOG_ERROR("Channel {} has no packet queue in LocalOnly state", ctx.name);
            enet_packet_destroy(packet);
        }
    }

    void RemoteOnly::enter(ChannelFSMRoot::Control& control) {
        auto& ctx = control.context();

        // Allocate packet queue for packets arriving before local channel creation
        if (!ctx.packet_queue) {
            ctx.packet_queue = std::make_unique<moodycamel::ConcurrentQueue<ENetPacket*>>();
        }

        LOG_TRACE("Channel {} entered RemoteOnly state (waiting for local creation)", ctx.name);
    }

    void RemoteOnly::handle_packet(ENetPacket* packet, ChannelFSMRoot::Control& control) {
        auto& ctx = control.context();

        // Queue packet for later processing when local channel is created
        if (ctx.packet_queue) {
            ctx.packet_queue->enqueue(packet);
            LOG_TRACE("Channel {} queued packet (RemoteOnly)", ctx.name);
        } else {
            // This shouldn't happen, but handle gracefully
            LOG_ERROR("Channel {} has no packet queue in RemoteOnly state", ctx.name);
            enet_packet_destroy(packet);
        }
    }

    void Linked::enter(ChannelFSMRoot::Control& control) {
        auto& ctx = control.context();

        // Drain any queued packets from RemoteOnly state
        if (ctx.packet_queue && ctx.local_channel) {
            ENetPacket* packet;
            while (ctx.packet_queue->try_dequeue(packet)) {
                LOG_TRACE("Channel {} draining queued packet", ctx.name);
                ctx.local_channel->take_packet(packet);
            }

            // Queue no longer needed
            ctx.packet_queue.reset();
        }

        LOG_DEBUG("Channel {} linked (both local and remote exist)", ctx.name);
    }

    void Linked::handle_packet(ENetPacket* packet, ChannelFSMRoot::Control& control) {
        auto& ctx = control.context();

        // Route directly to local channel
        if (ctx.local_channel) {
            ctx.local_channel->take_packet(packet);
        } else {
            // This shouldn't happen in Linked state
            LOG_ERROR("Channel {} in Linked state but local_channel is null", ctx.name);
            enet_packet_destroy(packet);
        }
    }

} // namespace v
