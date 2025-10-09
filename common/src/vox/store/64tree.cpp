//
// Created by niooi on 10/7/2025.
//

#include <vox/store/64tree.h>

namespace v {
    using Type = S64Node::Type;

    void Sparse64Tree::clear_node(S64Node_UP& node)
    {
        u64 m = node->child_mask;

        for (auto i : node->child_indices())
        {
            clear_node(node->children[i]);
        }

        node.reset();
    }

    void Sparse64Tree::fill_node(S64Node_UP& node, VoxelType t)
    {
        node->type = Type::SingleTypeLeaf;

        // destroy all children
        std::vector<S64Node_UP>().swap(node->children);

        // destroy all current voxel info
        node->voxels = { t };

        node->child_mask = 0b1;
    }

    VoxelType Sparse64Tree::voxel_at(const glm::vec3& pos) const
    {
        S64Node_P curr = root_.get();
        // all fields of max are the same, and max.x y or z contains the per-axis extent
        // of our tree along each axis.
        //
        // the divisor in the algo is 1u << shift_amt
        // example:
        // for a tree with an extent of 64, the divisor is 16. 
        // we grab the appropriate node via u_pos / 16, then flattening into an index. 
        // then, when we modulo u_pos to shift it into the next node's space,
        // we grab the divisor via (1u << shift_amt), which in this case is 4, and 1 << 4 == 16.
        // then shift_amt -= 2, so on the next iteration we divide by 16, and then mod 16.
        // when shift_amt == 0, we are on a leaf, if the tree is not malformed.
        // then u_pos / (1 << 0) is a no-op.
        u8         shift_amt = CTZ(static_cast<u32>(bounds_.max.x) >> 2);
        glm::uvec3 u_pos{ pos };

        // TODO! assert any component of pos is not >= the extent. 

        // divisor should never reach 0 if everythign is implemented properly
        // TODO! this algo has room for improvement. i feel it.. i smeelllll iiiiittttt...
        while (1)
        {
            // TODO! just use the lowest 3 bits?? or am i stupid
            // is that the same as shifting and doing trhis every time
            u8 idx = static_cast<u8>(curr->get_idx(
                u_pos.x >> shift_amt, u_pos.y >> shift_amt, u_pos.z >> shift_amt));

            switch (curr->type)
            {
            case Type::SingleTypeLeaf:
                return curr->voxels[0];
            case Type::Leaf:
                return curr->voxels[idx];
            default:
            }

            if ((curr->child_mask & (1ull << idx)) == 0)
                // child don't exist. kill (implicit air, or whatever 0 means)
                return 0;

            curr = curr->children[idx].get();

            // transform the coordinates into local coordinates for the next node

            // take u_pos % divisor 
            u32 b = (1u << shift_amt) - 1;
            u_pos.x &= b;
            u_pos.y &= b;
            u_pos.z &= b;

            if (!shift_amt)
                break;

            shift_amt -= 2;
        };

        LOG_ERROR("Unreachable path");
        return 0;
    }
} // namespace v
