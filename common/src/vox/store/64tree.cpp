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
        node->voxels = {t};

        node->child_mask = 0b1;
    }
} // namespace v
