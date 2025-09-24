//
// Lightweight test utilities for engine subsystems
//

#pragma once

#include <defs.h>
#include <engine/engine.h>
#include <prelude.h>

namespace v::testing {
    struct TestContext {
        const char* name{ "test" };
        u64         checks{ 0 };
        u64         failures{ 0 };
    };

    // Initialize logging/time and return a fresh engine
    inline std::unique_ptr<Engine> init_test(const char* name = "vtest")
    {
        v::init(name);
        return std::make_unique<Engine>();
    }

    // Assert that condition eventually becomes true before deadline_tick.
    // - Logs debug progress before the deadline, error afterwards.
    template <typename... Args>
    inline void expect_before(
        TestContext& ctx, Engine& engine, bool cond, u64 deadline_tick, const char* fmt,
        Args&&... args)
    {
        ctx.checks++;
        const u64 tick = engine.current_tick();
        if (!cond)
        {
            if (tick < deadline_tick)
            {
                const std::string msg =
                    fmt::vformat(fmt, fmt::make_format_args(std::forward<Args>(args)...));
                LOG_DEBUG("[{}][t={}] pending: {}", ctx.name, tick, msg);
            }
            else
            {
                ctx.failures++;
                const std::string msg =
                    fmt::vformat(fmt, fmt::make_format_args(std::forward<Args>(args)...));
                LOG_ERROR("[{}][t={}] FAILED: {}", ctx.name, tick, msg);
            }
        }
        else
        {
            const std::string msg =
                fmt::vformat(fmt, fmt::make_format_args(std::forward<Args>(args)...));
            LOG_TRACE("[{}][t={}] ok: {}", ctx.name, tick, msg);
        }
    }

    // Immediate assertion: increments failure count if condition is false.
    template <typename... Args>
    inline void assert_now(
        TestContext& ctx, Engine& /*engine*/, bool cond, const char* fmt, Args&&... args)
    {
        ctx.checks++;
        if (!cond)
        {
            ctx.failures++;
            const std::string msg =
                fmt::vformat(fmt, fmt::make_format_args(std::forward<Args>(args)...));
            LOG_ERROR("[{}] ASSERT FAILED: {}", ctx.name, msg);
        }
        else
        {
            const std::string msg =
                fmt::vformat(fmt, fmt::make_format_args(std::forward<Args>(args)...));
            LOG_TRACE("[{}] assert ok: {}", ctx.name, msg);
        }
    }
} // namespace v::testing
