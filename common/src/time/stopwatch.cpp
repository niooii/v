//
// Created by niooi on 7/28/2025.
//

#include <time/stopwatch.h>
#include <time/time.h>

namespace v {
    Stopwatch::Stopwatch() { prev = time::secs(); }

    Stopwatch::~Stopwatch() {}

    f64 Stopwatch::elapsed() { return time::secs() - prev; }

    f64 Stopwatch::until(f64 target)
    {
        return target - elapsed();
    }

    f64 Stopwatch::reset()
    {
        f64 current      = time::secs();
        f64 elapsed_time = current - prev;
        prev             = current;
        return elapsed_time;
    }
}
