//
// Created by niooi on 10/7/2025.
//

#include <vox/store/64tree.h>

namespace v {
    void Sparse64Tree::clear_node(S64Node_P& node)
    {
        u64 m = node->child_mask;

        while (m)
        {
            u32 idx = CTZ64(m);

            // clear bit
            m &= (m - 1);
        }
    }
} // namespace v
