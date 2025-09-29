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

        TestContext(const char* test_name = "test") : name(test_name)
        {
            LOG_INFO("[{}] test start", name);
        }

        ~TestContext()
        {
            if (failures > 0)
                LOG_ERROR("[{}] {} failures over {} checks", name, failures, checks);
            else
                LOG_INFO("[{}] PASS: {} checks", name, checks);
        }

        int is_failure() const { return failures > 0 ? 1 : 0; }
    };

    // Initialize core subsystems and return a fresh engine with test context
    inline std::pair<std::unique_ptr<Engine>, TestContext>
    init_test(const char* name = "vtest")
    {
        v::init(name);

        // construct in place
        return { std::piecewise_construct,
                 std::forward_as_tuple(std::make_unique<Engine>()),
                 std::forward_as_tuple(name) };
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
                LOG_TRACE("[{}][t={}] pending: {}", ctx.name, tick, msg);
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
