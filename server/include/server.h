#include <prelude.h>
#include <engine/contexts/net/ctx.h>
#include <stdexcept>
#include "domain/domain.h"
#include "engine/contexts/net/listener.h"

namespace v {
    struct ServerConfig {
        std::string host;
        u16 port;
    };

    /// A singleton server domain
    class ServerDomain : public SDomain<ServerDomain> {
    public:
        ServerDomain(ServerConfig& conf, Engine& engine, const std::string& name = "Server Domain") : SDomain(engine, name) {
            auto net_ctx = engine.get_context<NetworkContext>();
            if (!net_ctx) {
                net_ctx = engine.add_context<NetworkContext>(engine);
            }

            listener_ = net_ctx->listen_on(conf.host, conf.port);

            // register a bunch of on ticks and stuff

            LOG_INFO("Listening on {}:{}", conf.host, conf.port);
        }

    private:
        std::shared_ptr<NetListener> listener_;
    };
}
