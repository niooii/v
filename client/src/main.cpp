//
// Created by niooi on 7/24/25.
//

#include <iostream>
#include <prelude.h>
#include <time/time.h>
#include <time/stopwatch.h>
#include <contexts/window.h>

constexpr i32 TEMP_MAX_FPS = 60;

using namespace v;

int main()
{
    init();

    // test some time stuff
    auto stopwatch = Stopwatch();
    constexpr f64 TEMP_SPF = 1.0 / TEMP_MAX_FPS;

    Engine engine {};

    auto& window_ctx = engine.add_context<WindowingContext>();

    while (true)
    {
        LOG_TRACE("render frame lol");
        auto sleep_time = stopwatch.until(TEMP_SPF);
        if (sleep_time > 0)
            time::sleep_ms(sleep_time * 1000);

        stopwatch.reset();
    }

    return 0;
}
