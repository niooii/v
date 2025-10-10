// Random number generation integration tests

#include <algorithm>
#include <numeric>
#include <rand.h>
#include <test.h>
#include <vector>

using namespace v;

int main()
{
    auto [engine, tctx] = testing::init_test("rand");

    // Initialize and seed the RNG
    v::rand::init();
    v::rand::seed(12345);

    tctx.assert_now(v::rand::last_seed() == 12345, "Seed stored correctly");

    // Test basic integer generation
    {
        u64 val1 = v::rand::next_u64();
        u64 val2 = v::rand::next_u64();
        tctx.assert_now(val1 != val2, "next_u64() returns different values");
    }

    {
        u32 val1 = v::rand::next_u32();
        u32 val2 = v::rand::next_u32();
        tctx.assert_now(val1 != val2, "next_u32() returns different values");
    }

    // Test uniform real generation in [0, 1)
    {
        f64 val = v::rand::uniform();
        tctx.assert_now(val >= 0.0, "uniform() >= 0");
        tctx.assert_now(val < 1.0, "uniform() < 1");
    }

    // Test frange
    {
        f64 val = v::rand::frange(5.0, 10.0);
        tctx.assert_now(val >= 5.0, "frange() >= min");
        tctx.assert_now(val < 10.0, "frange() < max");

        // Test swapped bounds
        f64 val_swapped = v::rand::frange(10.0, 5.0);
        tctx.assert_now(val_swapped >= 5.0, "frange() handles swapped min/max");
        tctx.assert_now(val_swapped < 10.0, "frange() handles swapped min/max");
    }

    // Test irange
    {
        i64 val = v::rand::irange(1, 5);
        tctx.assert_now(val >= 1 && val <= 5, "irange() within bounds");

        // Test swapped bounds
        i64 val_swapped = v::rand::irange(5, 1);
        tctx.assert_now(
            val_swapped >= 1 && val_swapped <= 5, "irange() handles swapped bounds");
    }

    // Test urange
    {
        u64 val = v::rand::urange(3, 8);
        tctx.assert_now(val >= 3 && val <= 8, "urange() within bounds");

        // Test swapped bounds
        u64 val_swapped = v::rand::urange(8, 3);
        tctx.assert_now(
            val_swapped >= 3 && val_swapped <= 8, "urange() handles swapped bounds");
    }

    // Test chance (deterministic by checking many samples)
    {
        // Test edge cases
        tctx.assert_now(!v::rand::chance(0.0), "chance(0.0) always false");
        tctx.assert_now(v::rand::chance(1.0), "chance(1.0) always true");

        // Test that chance with 0.5 produces some true values over many trials
        // (avoid flaky test by using very permissive bounds)
        bool found_true  = false;
        bool found_false = false;

        for (int i = 0; i < 100; ++i)
        {
            if (v::rand::chance(0.5))
            {
                found_true = true;
            }
            else
            {
                found_false = true;
            }

            if (found_true && found_false)
                break;
        }

        tctx.assert_now(
            found_true && found_false,
            "chance(0.5) produces both true and false over time");
    }

    // Test pick iterator
    {
        std::vector<int> vec = { 1, 2, 3, 4, 5 };

        // Pick from non-empty range
        auto it = v::rand::pick(vec.begin(), vec.end());
        tctx.assert_now(
            it != vec.end(), "pick() from non-empty range returns valid iterator");
        tctx.assert_now(*it >= 1 && *it <= 5, "pick() returns value from range");

        // Test with empty range
        std::vector<int> empty;
        auto             empty_it = v::rand::pick(empty.begin(), empty.end());
        tctx.assert_now(empty_it == empty.end(), "pick() from empty range returns end()");
    }

    // Test distribution properties (deterministic bounds checking)
    {
        const int        samples = 1000;
        std::vector<int> results;

        // Generate samples and check they're within expected bounds
        for (int i = 0; i < samples; ++i)
        {
            int value = static_cast<int>(v::rand::frange(0, 10));
            tctx.assert_now(
                value >= 0 && value < 10, "frange produces values in correct range");
            results.push_back(value);
        }

        // Check that we get different values (avoid completely stuck RNG)
        std::sort(results.begin(), results.end());
        auto unique_end   = std::unique(results.begin(), results.end());
        int  unique_count = std::distance(results.begin(), unique_end);

        tctx.assert_now(unique_count > 1, "frange produces multiple different values");
    }

    // Test reproducibility
    {
        v::rand::seed(42);
        u64 val1 = v::rand::next_u64();
        u64 val2 = v::rand::next_u64();

        v::rand::seed(42);
        u64 val1_repro = v::rand::next_u64();
        u64 val2_repro = v::rand::next_u64();

        tctx.assert_now(val1 == val1_repro, "First value reproducible with same seed");
        tctx.assert_now(val2 == val2_repro, "Second value reproducible with same seed");
    }

    return tctx.is_failure();
}
