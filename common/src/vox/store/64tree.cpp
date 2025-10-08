//
// Created by niooi on 10/7/2025.
//

#include <vox/store/64tree.h>

namespace v {
    void Sparse64Tree::clear_node(S64Node_UP& node)
    {
        u64 m = node->child_mask;

        for (auto&& [i, _child] : node->child_iter()) {
            clear_node(node->children[i]);
        }

        node.reset();
    }
} // namespace v
