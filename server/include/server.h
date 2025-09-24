#include <engine/contexts/net/ctx.h>
#include <net/channels.h>
#include <prelude.h>
#include <stdexcept>
#include "domain/domain.h"
#include "engine/contexts/net/listener.h"

namespace v {
    struct ServerConfig {
        std::string host;
        u16         port;
    };

    /// A singleton server domain
    class ServerDomain : public SDomain<ServerDomain> {
    public:
        ServerDomain(
            ServerConfig& conf, Engine& engine,
            const std::string& name = "Server Domain") : SDomain(engine, name)
        {
            auto net_ctx = engine.get_context<NetworkContext>();
            if (!net_ctx)
            {
                LOG_WARN("Created default network context");
                net_ctx = engine.add_context<NetworkContext>(engine, 1 / 1000.0);
            }

            listener_ = net_ctx->listen_on(conf.host, conf.port);

            ServerComponent& server_comp = listener_->create_component(engine.entity());

            server_comp.on_connect = [&engine](std::shared_ptr<NetConnection> con)
            {
                LOG_INFO("Client connected successfully!");

                auto& connection_channel = con->create_channel<ConnectServerChannel>();

                // test some quick channel stuff via chat channel
                auto& cc = con->create_channel<ChatChannel>();

                auto& cc_comp = cc.create_component(engine.entity());

                cc_comp.on_recv = [&engine](const ChatMessage& msg)
                {
                    LOG_INFO("Got message {} from client", msg.msg);
                    // bounce to the other open channels (TODO! include exlcuding the
                    // current one somehow)
                    for (auto [e, channel] : engine.view<ChatChannel>().each())
                    {
                        ChatMessage payload = { .msg = msg.msg };
                        channel->send(payload);

                        LOG_TRACE("Echoed message to channel!");
                    }
                };
            };

            // register a bunch of on ticks and stuff

            LOG_INFO("Listening on {}:{}", conf.host, conf.port);
        }

    private:
        std::shared_ptr<NetListener> listener_;
    };
} // namespace v
