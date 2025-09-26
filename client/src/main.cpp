//
// Created by niooi on 7/24/25.
//

#include <contexts/render.h>
#include <engine/contexts/net/connection.h>
#include <engine/contexts/net/ctx.h>
#include <engine/contexts/window/sdl.h>
#include <engine/contexts/window/window.h>
#include <iostream>
#include <net/channels.h>
#include <prelude.h>
#include <time/stopwatch.h>
#include <time/time.h>
#include <world/world.h>

constexpr i32 TEMP_MAX_FPS = 60;

using namespace v;

int main(int argc, char** argv)
{
    init(argv[0]);

    // test some time stuff
    auto          stopwatch = Stopwatch();
    constexpr f64 TEMP_SPF  = 1.0 / TEMP_MAX_FPS;

    Engine engine{};

    // Shared world domain (client-side view/state)
    auto world = engine.add_domain<WorldDomain>(engine);

    auto sdl_ctx    = engine.add_context<SDLContext>(engine);
    auto window_ctx = engine.add_context<WindowContext>(engine);

    // Test event stuff
    auto window = window_ctx->create_window("hjey", { 600, 600 }, { 600, 600 });

    auto render_ctx = engine.add_context<RenderContext>(engine);

    auto net_ctx = engine.add_context<NetworkContext>(engine, 1.0 / 1000);

    LOG_INFO("Connecting to server...");
    auto connection = net_ctx->create_connection("127.0.0.1", 25566);

    LOG_INFO("Connection created, attempting to connect...");

    // window->capture_raw_input(true);
    auto lambda = [](glm::ivec2 _pos, glm::ivec2 rel_movement)
    { LOG_DEBUG("Mouse motion: {}, {}!", rel_movement.x, rel_movement.y); };

    // window->on_mouse_moved.connect<lambda>();

    auto& channel = connection->create_channel<ChatChannel>();
    auto& comp    = channel.create_component(engine.entity());

    ChatMessage msg;
    msg.msg = "hi server man";
    channel.send(msg);

    engine.on_tick.connect(
        {}, {}, "windows",
        [sdl_ctx, window_ctx]()
        {
            sdl_ctx->update();
            window_ctx->update();
            LOG_TRACE("UPdated window");
        });

    engine.on_tick.connect(
        { "windows" }, {}, "render",
        [render_ctx]
        {
            render_ctx->update();
        });

    engine.on_tick.connect({}, {}, "network", [net_ctx] { net_ctx->update(); });

    std::atomic_bool running{ true };

    SDLComponent& sdl_comp = sdl_ctx->create_component(engine.entity());
    sdl_comp.on_quit       = [&running] { running = false; };

    while (running)
    {
        engine.tick();

        if (const auto sleep_time = stopwatch.until(TEMP_SPF); sleep_time > 0)
            time::sleep_ms(sleep_time * 1000);

        stopwatch.reset();
    }

    return 0;
}
