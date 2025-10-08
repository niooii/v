// Time system integration tests

#include <chrono>
#include <thread>
#include <testing.h>
#include <time/time.h>
#include <time/stopwatch.h>

using namespace v;

int main()
{
    auto [engine, tctx] = testing::init_test("time");

    // Test sleep_ms precision
    {
        auto start = std::chrono::steady_clock::now();
        v::time::sleep_ms(100);
        auto end = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        tctx.assert_now(elapsed.count() >= 90, "sleep_ms slept for at least 90ms");
        tctx.assert_now(elapsed.count() <= 150, "sleep_ms didn't sleep excessively long");
    }

    // Test Stopwatch basic functionality
    {
        Stopwatch sw;
        tctx.assert_now(sw.reset() == 0.0, "Initial reset returns 0");

        v::time::sleep_ms(50);
        double elapsed = sw.reset();
        tctx.assert_now(elapsed >= 0.04, "Stopwatch measured at least 40ms");
        tctx.assert_now(elapsed <= 0.08, "Stopwatch didn't overmeasure");
    }

    // Test Stopwatch measurements
    {
        Stopwatch sw;
        v::time::sleep_ms(100);

        double first = sw.elapsed();
        v::time::sleep_ms(50);
        double second = sw.elapsed();

        tctx.assert_now(second > first, "Stopwatch time increases monotonically");
        tctx.assert_now((second - first) >= 0.04, "Second measurement shows at least 40ms elapsed");
    }

    // Test multiple concurrent Stopwatches
    {
        Stopwatch sw1, sw2;

        v::time::sleep_ms(50);
        double t1 = sw1.elapsed();
        double t2 = sw2.elapsed();

        // Should be approximately equal
        tctx.assert_now(std::abs(t1 - t2) < 0.01, "Concurrent stopwatches measure similar time");
    }

    return tctx.is_failure();
}