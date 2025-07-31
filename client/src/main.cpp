//
// Created by niooi on 7/24/25.
//

#include <contexts/render.h>
#include <contexts/window.h>
#include <domain/test.h>
#include <iostream>
#include <prelude.h>
#include <time/stopwatch.h>
#include <time/time.h>

constexpr i32 TEMP_MAX_FPS = 10;

using namespace v;

int main()
{
    init();

    // test some time stuff
    auto          stopwatch = Stopwatch();
    constexpr f64 TEMP_SPF  = 1.0 / TEMP_MAX_FPS;

    Engine engine{};

    auto window_ctx = engine.add_context<WindowContext>(engine);
    engine.add_context<RenderContext>(engine);

    // 8 example CountTo10Domain instances
    for (i32 i = 0; i < 8; ++i)
    {
        engine.add_domain<CountTo10Domain>(
            engine, "CountTo10Domain_" + std::to_string(i));
    }

    // Test event stuff
    auto window      = window_ctx->create_window("hjey", { 600, 600 }, { 600, 600 });
    auto whasgoingon = window_ctx->create_window("hi", { 600, 600 }, { 1200, 600 });

    window->capture_raw_input(true);
    auto lambda = [](glm::ivec2 _pos, glm::ivec2 rel_movement)
    { LOG_DEBUG("Mouse motion: {}, {}!", rel_movement.x, rel_movement.y); };

    window->on_mouse_moved.connect<lambda>();
    whasgoingon->on_mouse_moved.connect<lambda>();

    while (true)
    {
        engine.tick();
        window_ctx->update();

        // Example domain usage: CountTo10Domain counts to 10, and then
        // destroys itself.
        {
            auto reg = engine.registry_read();
            for (auto view = reg->view<CountTo10Domain*>().each();
                 auto [entity, domain] : view)
            {
                domain->update();
            }
        }

        // bunch of game logic here (GameContext? IDK)

        LOG_INFO("DT: {}", engine.delta_time());

        if (const auto sleep_time = stopwatch.until(TEMP_SPF); sleep_time > 0)
            time::sleep_ms(sleep_time * 1000);

        stopwatch.reset();
    }


    return 0;
}
