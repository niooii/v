//
// Created by niooi on 7/28/2025.
//

#pragma once

#include <defs.h>

namespace v {
    /// A stopwatch with nanosecond precision
    class Stopwatch {
    public:
        Stopwatch();
        ~Stopwatch();

        /// @return The time elapsed in seconds
        f64 elapsed();

        /// @param target The target value, in seconds
        /// @return The amount of seconds until the stopwatch reaches the
        /// specified target. This may be negative if the elapsed time is
        /// greater than the target value
        f64 until(f64 target);

        /// @return The time elapsed in seconds
        f64 reset();

    private:
        f64 prev;
    };
}
