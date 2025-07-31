//
// Created by niooi on 7/31/2025.
//

#define ENET_IMPLEMENTATION
#include <defs.h>
#include <engine/contexts/network.h>
#include <stdexcept>

namespace v {
    NetworkContext::NetworkContext(Engine& engine, const std::string& host_ip, u16 port) 
        : Context(engine), host_(nullptr) {
        
        if (enet_initialize() != 0) {
            throw std::runtime_error("Failed to initialize ENet");
        }

        ENetAddress address;
        if (enet_address_set_host(&address, host_ip.c_str()) != 0) {
            enet_deinitialize();
            throw std::runtime_error("Failed to set host address");
        }
        address.port = port;

        host_ = enet_host_create(&address, 32, 2, 0, 0);
        if (host_ == nullptr) {
            enet_deinitialize();
            throw std::runtime_error("Failed to create ENet host");
        }
    }

    NetworkContext::~NetworkContext() {
        if (host_ != nullptr) {
            enet_host_destroy(host_);
        }
        enet_deinitialize();
    }

    void NetworkContext::update() {
        
    }

} // namespace v
