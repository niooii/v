//
// Created by niooi on 7/24/25.
//

#include <contexts/window.h>
#include <iostream>
#include <prelude.h>
#include <time/stopwatch.h>
#include <time/time.h>

constexpr i32 TEMP_MAX_FPS = 60000;

using namespace v;

int main()
{
    init();

    // test some time stuff
    auto          stopwatch = Stopwatch();
    constexpr f64 TEMP_SPF  = 1.0 / TEMP_MAX_FPS;

    Engine engine{};

    auto& window_ctx = engine.add_context<WindowContext>();

    auto window =
        window_ctx.create_window("hjey", { 600, 600 }, { 600, 600 });
    auto whasgoingon =
        window_ctx.create_window("hi", { 600, 600 }, { 1200, 600 });

    auto lambda = [](glm::uvec2 vec)
    { LOG_DEBUG("Window resized: {}, {}!", vec.x, vec.y); };
    window->on_resize.connect<lambda>();

    while (true)
    {
        window_ctx.update();

        // logic blah blah
        // if (window->is_key_down(Key::Backspace))
        //     LOG_TRACE("Backspace");

        if (const auto sleep_time = stopwatch.until(TEMP_SPF);
            sleep_time > 0)
            time::sleep_ms(sleep_time * 1000);

        stopwatch.reset();
    }

    return 0;
}
