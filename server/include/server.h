#include <engine/contexts/net/ctx.h>
#include <net/channels.h>
#include <prelude.h>
#include <stdexcept>
#include "engine/contexts/net/listener.h"
#include "engine/domain.h"

namespace v {
    struct ServerConfig {
        std::string host;
        u16         port;
    };

    /// A singleton server domain
    class ServerDomain : public SDomain<ServerDomain> {
    public:
        ServerDomain(
            Engine& engine, ServerConfig& conf,
            const std::string& name = "Server Domain") : SDomain(engine, name)
        {
            auto net_ctx = engine.get_ctx<NetworkContext>();
            if (!net_ctx)
            {
                LOG_WARN("Created default network context");
                net_ctx = engine.add_ctx<NetworkContext>(1 / 500.0);
            }

            listener_ = net_ctx->listen_on(conf.host, conf.port);

            ServerComponent& server_comp = listener_->create_component(entity_);

            server_comp.on_connect = [this, &engine](std::shared_ptr<NetConnection> con)
            {
                LOG_INFO("Client connected successfully!");

                auto& connection_channel = con->create_channel<ConnectServerChannel>();

                auto& conn_comp = connection_channel.create_component(entity_);

                conn_comp.on_recv = [](const ConnectServerChannel::PayloadT& req)
                { LOG_INFO("New player {}", req.uuid); };

                // test some quick channel stuff via chat channel
                auto& cc = con->create_channel<ChatChannel>();

                auto& cc_comp = cc.create_component(entity_);

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
