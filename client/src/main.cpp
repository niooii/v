//
// Created by niooi on 7/24/25.
//

#include <contexts/window.h>
#include <contexts/render.h>
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

        // bunch of game logic here (GameContext? idk)

        LOG_INFO("DT: {}", engine.delta_time());

        if (const auto sleep_time = stopwatch.until(TEMP_SPF); sleep_time > 0)
            time::sleep_ms(sleep_time * 1000);

        stopwatch.reset();
    }



    return 0;
}
