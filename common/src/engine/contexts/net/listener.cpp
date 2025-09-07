#include <enet.h>
#include <engine/contexts/net/ctx.h>
#include <engine/contexts/net/listener.h>
#include <stdexcept>
#include "defs.h"

namespace v {
    NetListener::NetListener(
        NetworkContext* ctx, const std::string& host, u16 port, u32 max_connections) :
        net_ctx_(ctx)
    {
        ENetAddress address = { 0 };
        if (int res = enet_address_set_host(&address, host.c_str()); res < 0)
        {
            LOG_ERROR("Bad host: {}", host.c_str());
            throw std::runtime_error("Bad host");
        }

        address.port = port;
        host_        = enet_host_create(&address, max_connections, 4, 0, 0);
        if (!host_)
        {
            LOG_ERROR("Failed to create server host: {}:{}", host.c_str(), port);
            throw std::runtime_error("Bye");
        }
    }


    ServerComponent& NetListener::create_component(entt::entity id)
    {
        return net_ctx_->engine_.registry().emplace<ServerComponent>(id);
    }

    void NetListener::handle_new_connection(std::shared_ptr<NetConnection> con)
    {
        auto view = net_ctx_->engine_.registry().view<ServerComponent>();

        for (auto [entity, comp] : view.each())
        {
            if (comp.on_connect)
                comp.on_connect(con);
        }
    }
} // namespace v
