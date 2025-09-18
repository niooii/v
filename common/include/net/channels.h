//
// Created by niooi on 9/15/2025.
//

// A bunch of channels shared by client and server

#pragma once

#include <defs.h>
#include <domain/context.h>
#include <domain/domain.h>

#include <engine/contexts/net/channel.h>
#include <engine/contexts/net/ctx.h>
#include <engine/serial/serde.h>

namespace v {
    struct ConnectionRequest {
        // TODO! this is the name for now, no uuid stuff
        std::string uuid;

        SERIALIZE_FIELDS(uuid);

        SERDE_IMPL(ConnectionRequest);
    };

    /// Requests a connection from a player to the server
    class ConnectServerChannel
        : public v::NetChannel<ConnectServerChannel, ConnectionRequest> {};

    struct ChatMessage {
        std::string msg;

        SERIALIZE_FIELDS(msg);

        SERDE_IMPL(ChatMessage);
    };

    class ChatChannel : public NetChannel<ChatChannel, ChatMessage> {};
} // namespace v
