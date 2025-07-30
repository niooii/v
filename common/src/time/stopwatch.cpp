//
// Created by niooi on 7/28/2025.
//

#include <time/stopwatch.h>
#include <time/time.h>

namespace v {
    Stopwatch::Stopwatch() { prev_ = time::secs(); }

    Stopwatch::~Stopwatch() = default;

    f64 Stopwatch::elapsed() const { return time::secs() - prev_; }

    f64 Stopwatch::until(const f64 target) const { return target - elapsed(); }

    f64 Stopwatch::reset()
    {
        const f64 current      = time::secs();
        const f64 elapsed_time = current - prev_;
        prev_                  = current;

        return elapsed_time;
    }
} // namespace v
