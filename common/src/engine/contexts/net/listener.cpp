#include <engine/contexts/net/listener.h>
#include <engine/contexts/net/ctx.h>
#include <enet.h>
#include <stdexcept>
#include "defs.h"

namespace v {
    NetListener::NetListener(NetworkContext* ctx, std::string& host, u16 port, u32 max_connections) : net_ctx_(ctx) {
        ENetAddress address = {0};
        if (int res = enet_address_set_host(&address, host.c_str()); res < 0) {
            LOG_ERROR("Bad host: {}", host.c_str());
            throw std::runtime_error("Bad host");
        }

        address.port = port; 
        host_ = enet_host_create(&address, max_connections, 4, 0, 0);
        if (!host_) {
            LOG_ERROR("Failed to create server host: {}:{}", host.c_str(), port);
            throw std::runtime_error("Bye");
        }
    }
    
    void NetListener::update() {
        ENetEvent event;
        
        while (enet_host_service(host_, &event, 0) > 0) {
            // TODO!
            switch (event.type) {

            }
        }
    }
}
