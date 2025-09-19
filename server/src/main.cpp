//
// Created by niooi on 7/24/25.
//

#include <engine/contexts/net/connection.h>
#include <server.h>
#include <engine/contexts/net/ctx.h>
#include <iostream>
#include <net/channels.h>
#include <prelude.h>
#include <time/stopwatch.h>
#include <time/time.h>
#include "engine/contexts/net/listener.h"

using namespace v;

constexpr f64 SERVER_UPDATE_RATE = 1.0 / 60.0; // 60 FPS

int main(int argc, char** argv)
{
    init(argv[0]);

    LOG_INFO("Starting v server on 127.0.0.1:25566");

    Engine engine{};

    // attempts to update every 1ms
    auto net_ctx = engine.add_context<NetworkContext>(engine, 1.0 / 1000.0);

    ServerConfig config{"127.0.0.1", 25566};
    engine.add_domain<ServerDomain>(config, engine);

    Stopwatch        stopwatch{};
    std::atomic_bool running{ true };

    LOG_INFO("Server ready, waiting for connections...");

    while (running)
    {
        stopwatch.reset();

        // Update network context to process events
        net_ctx->update();

        // Update engine
        engine.tick();

        // Sleep to maintain update rate
        if (const auto sleep_time = stopwatch.until(SERVER_UPDATE_RATE); sleep_time > 0)
            time::sleep_ms(sleep_time * 1000);
    }

    LOG_INFO("Server shutting down");
    return 0;
}
