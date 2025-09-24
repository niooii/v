// Unit-like checks for SparseVoxelOctree128

#include <containers/svo.h>
#include <testing.h>
#include <time/time.h>

using namespace v;

int main()
{
    auto                 engine = testing::init_test("vtest_svo");
    testing::TestContext tctx{ .name = "svo" };

    SparseVoxelOctree128 svo;

    testing::assert_now(tctx, *engine, svo.is_empty(), "new tree is empty");
    testing::assert_now(tctx, *engine, svo.node_count() == 0, "no nodes initially");

    // set a single voxel and verify only that one is set
    svo.set(5, 6, 7, 42);
    testing::assert_now(tctx, *engine, svo.get(5, 6, 7) == 42, "value set");
    testing::assert_now(tctx, *engine, svo.get(0, 0, 0) == 0, "other voxel empty");

    // clear it and ensure collapse to empty
    svo.set(5, 6, 7, 0);
    testing::assert_now(tctx, *engine, svo.get(5, 6, 7) == 0, "value cleared");
    testing::assert_now(tctx, *engine, svo.is_empty(), "tree empty after clear");

    // write a small block and spot check some values
    for (int x = 0; x < 8; ++x)
        for (int y = 0; y < 8; ++y)
            for (int z = 0; z < 8; ++z)
                svo.set(x, y, z, 7);

    testing::assert_now(tctx, *engine, svo.get(0, 0, 0) == 7, "block write ok");
    testing::assert_now(tctx, *engine, svo.get(7, 7, 7) == 7, "block write corner ok");
    testing::assert_now(tctx, *engine, svo.get(9, 9, 9) == 0, "outside block empty");

    // rewrite the block to zero and ensure it collapses
    for (int x = 0; x < 8; ++x)
        for (int y = 0; y < 8; ++y)
            for (int z = 0; z < 8; ++z)
                svo.set(x, y, z, 0);

    testing::assert_now(tctx, *engine, svo.get(0, 0, 0) == 0, "block cleared");
    testing::assert_now(tctx, *engine, svo.is_empty(), "tree empty after block clear");

    if (tctx.failures > 0)
    {
        LOG_ERROR("[svo] {} failures over {} checks", tctx.failures, tctx.checks);
        return 1;
    }

    LOG_INFO("[svo] PASS: {} checks", tctx.checks);
    return 0;
}
