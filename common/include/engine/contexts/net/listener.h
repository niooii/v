//
// Created by niooi on 7/31/2025.
//

#pragma once

#include <defs.h>
#include <enet.h>

namespace v {
    class NetListener {
        friend class NetworkContext;
    private:
        NetListener(class NetworkContext* ctx, std::string& host, u16 port, u32 max_connections = 128);
        void update();

        NetworkContext* net_ctx_;

        ENetHost* host_;

        std::string addr;
        u16 port;
    };
}
