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
        Engine&     engine;

        TestContext(Engine& eng, const char* test_name = "test") :
            name(test_name), engine(eng)
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

        // Assert that condition eventually becomes true before deadline_tick.
        // - Logs debug progress before the deadline, error afterwards.
        template <typename... Args>
        void expect_before(bool cond, u64 deadline_tick, const char* fmt, Args&&... args)
        {
            checks++;
            const u64 tick = engine.current_tick();
            if (!cond)
            {
                if (tick < deadline_tick)
                {
                    const std::string msg = fmt::vformat(
                        fmt, fmt::make_format_args(std::forward<Args>(args)...));
                    LOG_TRACE("[{}][t={}] pending: {}", name, tick, msg);
                }
                else
                {
                    failures++;
                    const std::string msg = fmt::vformat(
                        fmt, fmt::make_format_args(std::forward<Args>(args)...));
                    LOG_ERROR("[{}][t={}] FAILED: {}", name, tick, msg);
                }
            }
            else
            {
                const std::string msg =
                    fmt::vformat(fmt, fmt::make_format_args(std::forward<Args>(args)...));
                LOG_TRACE("[{}][t={}] ok: {}", name, tick, msg);
            }
        }

        // Immediate assertion: increments failure count if condition is false.
        template <typename... Args>
        void assert_now(bool cond, const char* fmt, Args&&... args)
        {
            checks++;
            if (!cond)
            {
                failures++;
                const std::string msg =
                    fmt::vformat(fmt, fmt::make_format_args(std::forward<Args>(args)...));
                LOG_ERROR("[{}] ASSERT FAILED: {}", name, msg);
            }
            else
            {
                const std::string msg =
                    fmt::vformat(fmt, fmt::make_format_args(std::forward<Args>(args)...));
                LOG_TRACE("[{}] assert ok: {}", name, msg);
            }
        }
    };

    // Initialize core subsystems and return a fresh engine with test context
    inline std::pair<std::unique_ptr<Engine>, TestContext>
    init_test(const char* name = "vtest")
    {
        v::init(name);

        auto    engine_ptr = std::make_unique<Engine>();
        Engine& engine_ref = *engine_ptr;
        return { std::piecewise_construct, std::forward_as_tuple(std::move(engine_ptr)),
                 std::forward_as_tuple(engine_ref, name) };
    }
} // namespace v::testing
