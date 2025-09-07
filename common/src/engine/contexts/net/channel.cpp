//
// Created by niooi on 9/7/2025.
//

#include <cstring>
#include <enet.h>
#include <engine/contexts/net/channel.h>
#include <engine/contexts/net/connection.h>
#include <engine/contexts/net/ctx.h>

namespace v {
    template <typename Derived, typename Payload>
    void NetChannel<Derived, Payload>::send(char* buf, u64 len)
    {
        u32 channel_id = runtime_type_id<Derived>();

        // create packet with u32 prefix + payload data
        const u64   total_len = sizeof(u32) + len;
        ENetPacket* packet =
            enet_packet_create(nullptr, total_len, ENET_PACKET_FLAG_RELIABLE);

        if (!packet)
        {
            LOG_ERROR("Failed to create packet for channel {}", Derived::unique_name());
            return;
        }

        // write channel ID as first 4 bytes
        std::memcpy(packet->data, &channel_id, sizeof(u32));

        // write payload data after the channel ID
        std::memcpy(packet->data + sizeof(u32), buf, len);

        // send the packet
        if (enet_peer_send(conn_->peer_, 0, packet) != 0)
        {
            u32 state = conn_->peer_->state;
            if (state == ENET_PEER_STATE_CONNECTING || state == ENET_PEER_STATE_CONNECTION_PENDING || state == ENET_PEER_STATE_CONNECTION_SUCCEEDED) {
                LOG_WARN("Connection is not yet open, queueing packet send");
                LOG_ERROR("TODO! Packet queueing not implemented yet");
                // TODO! when implementing make sure that enet_packet_destroy isnt called
            } else {
                LOG_ERROR("Failed to send packet on channel {}", Derived::unique_name());
            }
            enet_packet_destroy(packet);
        }
    }
} // namespace v
