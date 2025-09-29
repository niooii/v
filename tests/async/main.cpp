// Async context integration test

#include <chrono>
#include <engine/contexts/async/async.h>
#include <engine/test.h>
#include <testing.h>
#include <thread>

using namespace v;

int main()
{
    auto [engine, tctx] = testing::init_test("async");

    engine->add_ctx<AsyncContext>(*engine, 4);
    auto* async_ctx = engine->get_ctx<AsyncContext>();

    // task creation and execution
    {
        bool task_executed = false;
        auto future        = async_ctx->task(
            [&task_executed]()
            {
                task_executed = true;
                return 42;
            });

        // wait for task to complete
        future.wait();
        int result = future.get();

        testing::assert_now(tctx, *engine, task_executed, "Task function executed");
        testing::assert_now(tctx, *engine, result == 42, "Task returned correct value");
    }

    // multiple concurrent tasks
    {
        constexpr int            num_tasks = 10;
        std::vector<Future<int>> futures;
        std::atomic<int>         tasks_executed{ 0 };

        for (int i = 0; i < num_tasks; ++i)
        {
            futures.emplace_back(async_ctx->task(
                [&tasks_executed, i]()
                {
                    // Simulate some work
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    tasks_executed.fetch_add(1);
                    return i * 2;
                }));
        }

        // Wait for all tasks
        for (auto& future : futures)
        {
            future.wait();
        }

        // Verify all tasks executed
        testing::assert_now(
            tctx, *engine, tasks_executed.load() == num_tasks,
            "All concurrent tasks executed");

        // Verify results
        for (int i = 0; i < num_tasks; ++i)
        {
            int result = futures[i].get();
            testing::assert_now(
                tctx, *engine, result == i * 2,
                fmt::format("Task {} returned correct value", i).c_str());
        }
    }

    // wait_for()
    {
        auto future = async_ctx->task(
            []()
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                return 123;
            });

        // Should timeout (50ms < 100ms sleep)
        auto start = std::chrono::steady_clock::now();
        future.wait_for(std::chrono::milliseconds(50));
        auto elapsed = std::chrono::steady_clock::now() - start;

        testing::assert_now(
            tctx, *engine, elapsed >= std::chrono::milliseconds(40),
            "wait_for() respected timeout");

        // Now wait for completion
        future.wait();
        int result = future.get();
        testing::assert_now(tctx, *engine, result == 123, "Task completed after timeout");
    }

    // different return types
    {
        // String task
        auto string_future = async_ctx->task([]() { return std::string("hello world"); });
        string_future.wait();
        auto str_result = string_future.get();
        testing::assert_now(
            tctx, *engine, str_result == "hello world",
            "String task returned correct value");

        // Void task
        bool actual_void_executed = false;
        auto void_future =
            async_ctx->task([&actual_void_executed]() { actual_void_executed = true; });
        void_future.wait();
        testing::assert_now(tctx, *engine, actual_void_executed, "Void task executed");

        // Test void task with .then() callback
        bool void_callback_executed = false;
        auto void_future_with_callback =
            async_ctx->task([&actual_void_executed]() { actual_void_executed = true; });
        void_future_with_callback.then([&void_callback_executed]()
                                       { void_callback_executed = true; });
        void_future_with_callback.wait();
        for (int i = 0; i < 10; ++i)
        {
            engine->tick();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            if (void_callback_executed)
                break;
        }
        testing::assert_now(
            tctx, *engine, void_callback_executed, "Void task .then() callback executed");
    }

    // long-running computation
    {
        auto future = async_ctx->task(
            []()
            {
                int sum = 0;
                for (int i = 0; i < 1000000; ++i)
                {
                    sum += i % 1000;
                }
                return sum;
            });

        future.wait();
        int result = future.get();
        testing::assert_now(tctx, *engine, result > 0, "Long computation completed");
    }

    // Exception handling tests
    {
        auto future = async_ctx->task(
            []()
            {
                throw std::runtime_error("Test exception");
                return 42;
            });

        // This should handle the exception gracefully
        try
        {
            future.wait();
            future.get(); // This should throw
            testing::assert_now(
                tctx, *engine, false, "Expected exception but none was thrown");
        }
        catch (const std::runtime_error& e)
        {
            testing::assert_now(
                tctx, *engine, std::string(e.what()) == "Test exception",
                "Exception propagated correctly");
        }
    }

    // .then() should not trigger on exception
    {
        bool callback_executed = false;
        auto future            = async_ctx->task(
            []()
            {
                throw std::runtime_error("Test exception");
                return 42;
            });

        future.then([&callback_executed](int result) { callback_executed = true; });

        try
        {
            future.wait();
        }
        catch (...)
        {
            // Expected exception
        }

        for (int i = 0; i < 10; ++i)
        {
            engine->tick();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        testing::assert_now(
            tctx, *engine, !callback_executed,
            ".then() callback not executed on exception");
    }

    // .or_else() should not trigger on success
    {
        bool error_callback_executed = false;
        auto future                  = async_ctx->task([]() { return 42; });

        future.or_else([&error_callback_executed](std::exception_ptr e)
                       { error_callback_executed = true; });

        future.wait();
        for (int i = 0; i < 10; ++i)
        {
            engine->tick();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        testing::assert_now(
            tctx, *engine, !error_callback_executed,
            ".or_else() callback not executed on success");
    }

    // .then() callback registration
    {
        bool callback_executed = false;
        int  callback_result   = 0;
        auto future            = async_ctx->task(
            []()
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                return 555;
            });

        future.then(
            [&callback_executed, &callback_result](int result)
            {
                callback_executed = true;
                callback_result   = result;
                // This should execute on the engine's main thread
            });

        // Wait and tick the engine to process callbacks
        future.wait();
        for (int i = 0; i < 10; ++i)
        {
            engine->tick();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            if (callback_executed)
                break;
        }

        testing::assert_now(
            tctx, *engine, callback_executed, ".then() callback executed");
        testing::assert_now(
            tctx, *engine, callback_result == 555,
            ".then() callback received correct value");
    }

    // .then() called after completion
    {
        bool callback_executed = false;
        auto future            = async_ctx->task([]() { return 777; });

        // Wait for completion first
        future.wait();

        // Then register callback - should execute immediately
        future.then([&callback_executed](int result) { callback_executed = true; });

        // Tick the engine to process immediate callback
        for (int i = 0; i < 5; ++i)
        {
            engine->tick();
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            if (callback_executed)
                break;
        }

        testing::assert_now(
            tctx, *engine, callback_executed,
            ".then() executed immediately when future already completed");
    }

    // .or_else() tests
    {
        bool error_callback_executed = false;
        auto future                  = async_ctx->task(
            []()
            {
                throw std::runtime_error("Test error");
                return 42;
            });

        future.or_else(
            [&error_callback_executed](std::exception_ptr e)
            {
                error_callback_executed = true;
                try
                {
                    std::rethrow_exception(e);
                }
                catch (const std::runtime_error& ex)
                {
                    // Expected exception type
                }
            });

        future.then([](int a) { LOG_INFO("yay"); });

        try
        {
            future.wait();
        }
        catch (...)
        {
            // Expected exception
        }

        for (int i = 0; i < 10; ++i)
        {
            engine->tick();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            if (error_callback_executed)
                break;
        }

        testing::assert_now(
            tctx, *engine, error_callback_executed, ".or_else() callback executed");
    }

    // .or_else() called after completion
    {
        bool error_callback_executed = false;
        auto future                  = async_ctx->task(
            []()
            {
                throw std::runtime_error("Test error");
                return 42;
            });

        // Wait for completion first
        try
        {
            future.wait();
        }
        catch (...)
        {
            // Expected exception
        }

        // Then register callback - should execute immediately
        future.or_else([&error_callback_executed](std::exception_ptr e)
                       { error_callback_executed = true; });

        // Tick the engine to process immediate callback
        for (int i = 0; i < 5; ++i)
        {
            engine->tick();
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            if (error_callback_executed)
                break;
        }

        testing::assert_now(
            tctx, *engine, error_callback_executed,
            ".or_else() executed immediately when future already completed with "
            "exception");
    }

    return tctx.is_failure();
}
