//
// Created by niooi on 7/24/25.
//

#include <iostream>
#include <prelude.h>
#include <time/time.h>
#include <time/stopwatch.h>

constexpr i32 TEMP_MAX_FPS = 60;

int main()
{
    v::init();

    // test some time stuff
    auto stopwatch = v::Stopwatch();
    constexpr f64 TEMP_SPF = 1.0 / TEMP_MAX_FPS;
    while (true)
    {
        LOG_TRACE("render frame lol");
        auto sleep_time = stopwatch.until(TEMP_SPF);
        if (sleep_time > 0)
            v::time::sleep_ms(sleep_time * 1000);

        stopwatch.reset();
    }

    return 0;
}
