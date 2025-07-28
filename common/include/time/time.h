//
// Created by niooi on 7/28/2025.
//

#pragma once

#include <defs.h>

/// The timekeeping namespace
namespace v::time {
    /// Initialize the timing system
    /// Must be called before using other timing functions
    void init();

    /// Get seconds elapsed since init() was called
    /// @return Time in seconds with nanosecond precision
    f64 secs();

    /// Get nanoseconds elapsed since init() was called
    /// @return Time in nanoseconds
    u64 ns();

    /// Get milliseconds elapsed since init() was called
    /// @return Time in milliseconds
    u64 ms();

    /// Get milliseconds since Unix epoch
    /// @return Milliseconds since Jan 1, 1970 UTC
    u64 epoch_ms();

    /// Get nanoseconds since Unix epoch
    /// @return Nanoseconds since Jan 1, 1970 UTC
    u64 epoch_ns();

    /// Halt the current thread for specified milliseconds
    /// @param ms Number of milliseconds to sleep
    void sleep_ms(u32 ms);

    /// Halt the current thread for specified nanoseconds
    /// @param ns Number of nanoseconds to sleep
    void sleep_ns(u64 ns);
}