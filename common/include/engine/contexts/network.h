//
// Created by niooi on 7/31/2025.
//

#pragma once

#include <domain/context.h>
#include <string>

extern "C" {
    #include <enet.h>
}

namespace v {
    /// NetworkContext is thread-safe
    class NetworkContext : public Context {
    public:
        explicit NetworkContext(Engine& engine, const std::string& host_ip = "0.0.0.0", u16 port = 7777);
        ~NetworkContext();

        void update();
        
    private:
        ENetHost* host_;
    };
}