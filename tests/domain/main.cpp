// Domain system integration test using CountTo10Domain

#include <domain/test.h>
#include <testing.h>

using namespace v;

int main()
{
    auto                 engine = testing::init_test("vtest_domain");
    testing::TestContext tctx{ .name = "domain" };

    // Create 8 example CountTo10Domain instances
    for (i32 i = 0; i < 8; ++i)
    {
        engine->add_domain<CountTo10Domain>(
            *engine, "CountTo10Domain_" + std::to_string(i));
    }

    // Verify all domains were created
    auto initial_count = engine->view<CountTo10Domain>().size();
    testing::assert_now(tctx, *engine, initial_count == 8, "8 domains created");

    bool all_domains_updated = false;
    u64  domain_count_zero   = 0;

    // Set up domain update loop - CountTo10Domain counts to 10, then destroys itself
    engine->on_tick.connect(
        {}, {}, "domain updates",
        [&]()
        {
            for (auto [entity, domain] : engine->view<CountTo10Domain>().each())
            {
                domain->update();
            }

            // Check if all domains have self-destructed
            auto current_count = engine->view<CountTo10Domain>().size();
            if (current_count == 0)
            {
                domain_count_zero++;
                if (domain_count_zero >= 3) // stable for a few ticks
                    all_domains_updated = true;
            }
        });

    constexpr u64 max_ticks = 2000; // test budget
    for (u64 i = 0; i < max_ticks; ++i)
    {
        engine->tick();

        // Progress check - domains should eventually self-destruct
        testing::expect_before(
            tctx, *engine, all_domains_updated, 1500, "all domains self-destructed");

        if (all_domains_updated)
            break;
    }

    // Final hard assert
    testing::assert_now(tctx, *engine, all_domains_updated, "domains completed lifecycle");
    testing::assert_now(tctx, *engine, engine->view<CountTo10Domain>().size() == 0, "no domains remain");

    if (tctx.failures > 0)
    {
        LOG_ERROR("[domain] {} failures over {} checks", tctx.failures, tctx.checks);
        return 1;
    }

    LOG_INFO("[domain] PASS: {} checks", tctx.checks);
    return 0;
}