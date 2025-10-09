#include <vox/store/64tree.h>
#include <testing.h>

using namespace v;

int main()
{
    auto [engine, tctx] = testing::init_test("64tree");

    {
        Sparse64Tree tree(3);
        tctx.assert_now(tree.get_voxel(0, 0, 0) == 0, "new tree returns air");
        tctx.assert_now(tree.get_voxel(10, 10, 10) == 0, "unset voxel is air");
    }

    {
        Sparse64Tree tree(3);
        tree.set_voxel(5, 6, 7, 42);
        tctx.assert_now(tree.get_voxel(5, 6, 7) == 42, "set and get single voxel");
        tctx.assert_now(tree.get_voxel(5, 6, 8) == 0, "adjacent voxel is air");
        tctx.assert_now(tree.get_voxel(0, 0, 0) == 0, "far voxel is air");
    }

    {
        Sparse64Tree tree(3);
        tree.set_voxel(5, 6, 7, 42);
        tree.set_voxel(5, 6, 7, 0);
        tctx.assert_now(tree.get_voxel(5, 6, 7) == 0, "clearing voxel works");
    }

    {
        Sparse64Tree tree(3);
        tree.set_voxel(5, 6, 7, 42);
        tree.set_voxel(5, 6, 7, 99);
        tctx.assert_now(tree.get_voxel(5, 6, 7) == 99, "overwriting voxel works");
    }

    {
        Sparse64Tree tree(3);
        for (u32 x = 0; x < 4; ++x)
            for (u32 y = 0; y < 4; ++y)
                for (u32 z = 0; z < 4; ++z)
                    tree.set_voxel(x, y, z, 7);

        tctx.assert_now(tree.get_voxel(0, 0, 0) == 7, "4x4x4 block filled correctly");
        tctx.assert_now(tree.get_voxel(3, 3, 3) == 7, "4x4x4 block corner filled");
        tctx.assert_now(tree.get_voxel(2, 1, 3) == 7, "4x4x4 block middle filled");
        tctx.assert_now(tree.get_voxel(4, 0, 0) == 0, "outside 4x4x4 block is air");
    }

    {
        Sparse64Tree tree(3);
        for (u32 x = 0; x < 4; ++x)
            for (u32 y = 0; y < 4; ++y)
                for (u32 z = 0; z < 4; ++z)
                    tree.set_voxel(x, y, z, 7);

        for (u32 x = 0; x < 4; ++x)
            for (u32 y = 0; y < 4; ++y)
                for (u32 z = 0; z < 4; ++z)
                    tree.set_voxel(x, y, z, 0);

        tctx.assert_now(tree.get_voxel(0, 0, 0) == 0, "cleared 4x4x4 block is air");
        tctx.assert_now(tree.get_voxel(2, 2, 2) == 0, "cleared 4x4x4 block middle is air");
    }

    {
        Sparse64Tree tree(3);
        for (u32 x = 0; x < 4; ++x)
            for (u32 y = 0; y < 4; ++y)
                for (u32 z = 0; z < 4; ++z)
                    tree.set_voxel(x, y, z, 7);

        tree.set_voxel(1, 1, 1, 0);
        tctx.assert_now(tree.get_voxel(1, 1, 1) == 0, "single voxel cleared in filled block");
        tctx.assert_now(tree.get_voxel(0, 0, 0) == 7, "other voxels still filled");
        tctx.assert_now(tree.get_voxel(3, 3, 3) == 7, "corner still filled");
    }

    {
        Sparse64Tree tree(3);
        for (u32 x = 0; x < 4; ++x)
            for (u32 y = 0; y < 4; ++y)
                for (u32 z = 0; z < 4; ++z)
                    tree.set_voxel(x, y, z, 7);

        tree.set_voxel(1, 1, 1, 99);
        tctx.assert_now(tree.get_voxel(1, 1, 1) == 99, "single voxel changed in uniform block");
        tctx.assert_now(tree.get_voxel(0, 0, 0) == 7, "other voxels unchanged");
    }

    {
        Sparse64Tree tree(4);
        tree.set_voxel(0, 0, 0, 1);
        tree.set_voxel(63, 63, 63, 2);
        tctx.assert_now(tree.get_voxel(0, 0, 0) == 1, "corner voxel set");
        tctx.assert_now(tree.get_voxel(63, 63, 63) == 2, "opposite corner voxel set");
        tctx.assert_now(tree.get_voxel(32, 32, 32) == 0, "middle voxel is air");
    }

    {
        Sparse64Tree tree(3);
        AABB region(glm::vec3(2, 2, 2), glm::vec3(6, 6, 6));
        tree.fill_aabb(region, 10);

        tctx.assert_now(tree.get_voxel(2, 2, 2) == 10, "aabb min filled");
        tctx.assert_now(tree.get_voxel(5, 5, 5) == 10, "aabb max-1 filled");
        tctx.assert_now(tree.get_voxel(3, 4, 5) == 10, "aabb middle filled");
        tctx.assert_now(tree.get_voxel(1, 2, 2) == 0, "outside aabb is air");
        tctx.assert_now(tree.get_voxel(6, 6, 6) == 0, "aabb max (exclusive) is air");
    }

    {
        Sparse64Tree tree(3);
        tree.fill_sphere(glm::vec3(8, 8, 8), 3.0f, 20);

        tctx.assert_now(tree.get_voxel(8, 8, 8) == 20, "sphere center filled");
        tctx.assert_now(tree.get_voxel(5, 8, 8) == 20, "sphere -x filled");
        tctx.assert_now(tree.get_voxel(10, 8, 8) == 20, "sphere +x filled");
        tctx.assert_now(tree.get_voxel(8, 5, 8) == 20, "sphere -y filled");
        tctx.assert_now(tree.get_voxel(2, 8, 8) == 0, "outside sphere -x is air");
        tctx.assert_now(tree.get_voxel(12, 8, 8) == 0, "outside sphere +x is air");
    }

    {
        Sparse64Tree tree(4);
        tree.fill_cylinder(glm::vec3(10, 10, 10), glm::vec3(10, 20, 10), 2.0f, 30);

        tctx.assert_now(tree.get_voxel(10, 10, 10) == 30, "cylinder base filled");
        tctx.assert_now(tree.get_voxel(10, 19, 10) == 30, "cylinder top filled");
        tctx.assert_now(tree.get_voxel(10, 15, 10) == 30, "cylinder middle filled");
        tctx.assert_now(tree.get_voxel(11, 15, 10) == 30, "cylinder +x within radius");
        tctx.assert_now(tree.get_voxel(13, 15, 10) == 0, "cylinder +x outside radius");
        tctx.assert_now(tree.get_voxel(10, 5, 10) == 0, "below cylinder is air");
        tctx.assert_now(tree.get_voxel(10, 25, 10) == 0, "above cylinder is air");
    }

    {
        Sparse64Tree tree(3);
        tree.fill_sphere(glm::vec3(8, 8, 8), 2.0f, 5);
        tree.fill_sphere(glm::vec3(8, 8, 8), 2.0f, 0);

        tctx.assert_now(tree.get_voxel(8, 8, 8) == 0, "clearing filled sphere works");
        tctx.assert_now(tree.get_voxel(7, 8, 8) == 0, "sphere cleared completely");
    }

    {
        Sparse64Tree tree(3);
        AABB region(glm::vec3(100, 100, 100), glm::vec3(110, 110, 110));
        tree.fill_aabb(region, 5);

        tctx.assert_now(tree.get_voxel(0, 0, 0) == 0, "out of bounds fill does nothing");
    }

    {
        Sparse64Tree tree(3);
        AABB region(glm::vec3(-5, -5, -5), glm::vec3(5, 5, 5));
        tree.fill_aabb(region, 7);

        tctx.assert_now(tree.get_voxel(0, 0, 0) == 7, "negative min clipped correctly");
        tctx.assert_now(tree.get_voxel(4, 4, 4) == 7, "clipped aabb filled");
    }

    {
        Sparse64Tree tree(2);
        for (u32 i = 0; i < 16; ++i)
        {
            tree.set_voxel(i, 0, 0, static_cast<u8>(i + 1));
        }

        for (u32 i = 0; i < 16; ++i)
        {
            tctx.assert_now(tree.get_voxel(i, 0, 0) == static_cast<u8>(i + 1), "linear voxels set correctly");
        }
    }

    {
        Sparse64Tree tree(3);
        for (u32 i = 0; i < 8; ++i)
            for (u32 j = 0; j < 8; ++j)
                for (u32 k = 0; k < 8; ++k)
                    tree.set_voxel(i, j, k, 42);

        tree.set_voxel(4, 4, 4, 99);
        tctx.assert_now(tree.get_voxel(4, 4, 4) == 99, "single voxel changed in large uniform region");
        tctx.assert_now(tree.get_voxel(0, 0, 0) == 42, "other voxels in region unchanged");
        tctx.assert_now(tree.get_voxel(7, 7, 7) == 42, "corner of region unchanged");
    }

    return tctx.is_failure();
}
