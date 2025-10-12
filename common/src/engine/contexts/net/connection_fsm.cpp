//
// Created by niooi on 10/11/2025.
//

#include <engine/contexts/net/connection_fsm.h>
#include <engine/contexts/net/connection.h>
#include <engine/contexts/net/ctx.h>
#include <engine/contexts/net/channel.h>

namespace v {
    void Connecting::enter(ConnectionFSMRoot::Control& control) {
        control.context().since_open.reset();
        LOG_TRACE("Connection entering Connecting state");
    }

    void Connecting::update(ConnectionFSMRoot::FullControl& control) {
        auto& ctx = control.context();

        // Skip timeout check if timeout is 0 or negative (disabled)
        if (ctx.connection_timeout <= 0.0) {
            return;
        }

        // Check for timeout
        if (ctx.since_open.elapsed() > ctx.connection_timeout) {
            LOG_ERROR("Connection timed out in {} seconds.", ctx.connection_timeout);
            ctx.remote_disconnected = true;

            // Queue immediate disconnect on IO thread
            ENetPeer* peer_ptr = ctx.peer;
            ctx.net_ctx->enqueue_io([peer_ptr] {
                enet_peer_disconnect_now(peer_ptr, 0);
            });

            // Transition to Disconnecting for cleanup
            control.changeTo<Disconnecting>();
        }
    }

    void Active::enter(ConnectionFSMRoot::Control& control) {
        auto& ctx = control.context();

        // Process any packets that arrived while pending
        ENetPacket* packet;
        while (ctx.pending_packets.try_dequeue(packet)) {
            ctx.connection->handle_raw_packet(packet);
        }

        // Send any packets that were queued to go out
        if (ctx.outgoing_packets) {
            ENetPacket* pkt;
            while (ctx.outgoing_packets->try_dequeue(pkt)) {
                enet_peer_send(ctx.peer, 0, pkt);
            }

            delete ctx.outgoing_packets;
            ctx.outgoing_packets = nullptr;
        }

        LOG_TRACE("Connection activated");
    }

    void Active::update(ConnectionFSMRoot::FullControl& control) {
        auto& ctx = control.context();

        // Update all channels (runs the callbacks for received/parsed data)
        for (auto& [name, chnl] : ctx.connection->c_insts_) {
            chnl->update();
        }

        // Destroy consumed packets
        ENetPacket* packet;
        while (ctx.connection->packet_destroy_queue_.try_dequeue(packet)) {
            enet_packet_destroy(packet);
        }
    }

    void Disconnecting::enter(ConnectionFSMRoot::Control& control) {
        auto& ctx = control.context();

        // Queue disconnect on IO thread if not already disconnected remotely
        if (!ctx.remote_disconnected) {
            ENetPeer* peer_ptr = ctx.peer;
            ctx.net_ctx->enqueue_io([peer_ptr] {
                enet_peer_disconnect(peer_ptr, 0);
            });
        }

        // Queue DestroyConnection event for main thread cleanup
        NetworkEvent event{
            NetworkEventType::DestroyConnection,
            ctx.connection->shared_con_,
            nullptr
        };
        ctx.net_ctx->event_queue_.enqueue(std::move(event));

        LOG_TRACE("Connection entering Disconnecting state");
    }

} // namespace v
