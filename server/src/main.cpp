//
// Created by niooi on 7/24/25.
//

#include <engine/contexts/net/ctx.h>
#include <iostream>
#include <prelude.h>
#include <time/stopwatch.h>
#include <time/time.h>

using namespace v;

constexpr f64 SERVER_UPDATE_RATE = 1.0 / 60.0; // 60 FPS

int main()
{
    init();

    LOG_INFO("Starting v server on 127.0.0.1:25566");

    Engine engine{};

    // Create network context with 120Hz update rate
    auto net_ctx = engine.add_context<NetworkContext>(engine, 1.0 / 120.0);

    // Listen on 127.0.0.1:25566
    auto listener = net_ctx->listen_on("127.0.0.1", 25566, 32);

    // Create server component to handle connections
    ServerComponent& server_comp = listener->create_component(engine.entity());

    // server_comp.on_connect = [](std::shared_ptr<NetConnection> con)
    // { LOG_INFO("Client connected successfully!"); };

    // server_comp.on_disconnect = [](std::shared_ptr<NetConnection> con)
    // { LOG_INFO("Client disconnected"); };

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
