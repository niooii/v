// Unit-like checks for SparseVoxelOctree128

#include <test.h>
#include <time/time.h>
#include <vox/store/svo.h>

using namespace v;

int main()
{
    auto [engine, tctx] = testing::init_test("svo");

    SparseVoxelOctree128 svo;

    tctx.assert_now(svo.is_empty(), "new tree is empty");
    tctx.assert_now(svo.node_count() == 0, "no nodes initially");

    // set a single voxel and verify only that one is set
    svo.set(5, 6, 7, 42);
    tctx.assert_now(svo.get(5, 6, 7) == 42, "value set");
    tctx.assert_now(svo.get(0, 0, 0) == 0, "other voxel empty");

    // clear it and ensure collapse to empty
    svo.set(5, 6, 7, 0);
    tctx.assert_now(svo.get(5, 6, 7) == 0, "value cleared");
    tctx.assert_now(svo.is_empty(), "tree empty after clear");

    // write a small block and spot check some values
    for (int x = 0; x < 8; ++x)
        for (int y = 0; y < 8; ++y)
            for (int z = 0; z < 8; ++z)
                svo.set(x, y, z, 7);

    tctx.assert_now(svo.get(0, 0, 0) == 7, "block write ok");
    tctx.assert_now(svo.get(7, 7, 7) == 7, "block write corner ok");
    tctx.assert_now(svo.get(9, 9, 9) == 0, "outside block empty");

    // rewrite the block to zero and ensure it collapses
    for (int x = 0; x < 8; ++x)
        for (int y = 0; y < 8; ++y)
            for (int z = 0; z < 8; ++z)
                svo.set(x, y, z, 0);

    tctx.assert_now(svo.get(0, 0, 0) == 0, "block cleared");
    tctx.assert_now(svo.is_empty(), "tree empty after block clear");

    return tctx.is_failure();
}
