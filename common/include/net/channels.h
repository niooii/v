//
// Created by niooi on 9/15/2025.
//

// A bunch of channels shared by client and server

#pragma once

#include <defs.h>
#include <domain/context.h>
#include <domain/domain.h>

#include <engine/contexts/net/ctx.h>
#include <engine/contexts/net/channel.h>
#include <engine/serial/serde.h>

namespace v {
    struct ConnectionRequest {
        // TODO! this is the name for now, no uuid stuff 
        std::string uuid;

        SERIALIZE_FIELDS(uuid);
    };
    /// Requests a connection from a player to the server
    class ConnectServerChannel : NetChannel<ConnectServerChannel, ConnectionRequest>{};
}
