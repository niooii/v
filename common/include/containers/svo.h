//
// Sparse Voxel Octree for 128x128x128 chunks
// Header-only implementation suitable for client and server
//

#pragma once

#include <array>
#include <cstddef>
#include <defs.h>
#include <memory>
#include <utility>

namespace v {

    /// A compact sparse voxel octree supporting 128^3 voxels.
    /// - Value type is `u16` (0 treated as empty)
    /// - Root covers a 128 cube; depth is 7 (since 2^7 = 128)
    /// - Automatically collapses homogeneous internal nodes into leaves
    class SparseVoxelOctree128 {
    public:
        using voxel_t                  = u16;
        static constexpr i32 size      = 128;
        static constexpr i32 max_depth = 7; // 2^7 = 128

        SparseVoxelOctree128() = default;
        ~SparseVoxelOctree128() { clear(); }

        /// Returns the voxel value at local coordinates [0,127]^3
        voxel_t get(i32 x, i32 y, i32 z) const
        {
            if (!root_)
                return 0;
            return get_at_node(root_, max_depth, x, y, z);
        }

        /// Sets the voxel value at local coordinates [0,127]^3
        /// Automatically creates/removes nodes and collapses when possible
        void set(i32 x, i32 y, i32 z, voxel_t v)
        {
            if (!root_ && v == 0)
            {
                // writing empty into empty tree: nothing to do
                return;
            }

            root_ = set_at_node(root_, max_depth, x, y, z, v);
        }

        /// Clear the entire tree to empty
        void clear()
        {
            destroy_node(root_);
            root_ = nullptr;
        }

        /// Returns approximate node count (for debugging)
        size_t node_count() const { return count_nodes(root_); }

        /// Returns true if tree is empty or entirely empty voxels
        bool is_empty() const
        {
            if (!root_)
                return true;
            if (root_->is_leaf)
                return root_->leaf() == 0;
            return false;
        }

    private:
        struct Node {
            bool is_leaf;
            union {
                voxel_t leaf_value; // valid if is_leaf == true
                struct {
                    u8    child_mask; // bit i set if children[i] exists
                    Node* children[8]; // valid if is_leaf == false
                } internal;
            } data{};

            // helpers for convenience
            voxel_t&       leaf() { return data.leaf_value; }
            const voxel_t& leaf() const { return data.leaf_value; }
            u8&            mask() { return data.internal.child_mask; }
            const u8&      mask() const { return data.internal.child_mask; }
            Node**         kids() { return data.internal.children; }
            Node* const*   kids() const { return data.internal.children; }
        };

        static Node* new_leaf(voxel_t v)
        {
            Node* n    = new Node();
            n->is_leaf = true;
            n->leaf()  = v;
            return n;
        }

        static Node* new_internal()
        {
            Node* n    = new Node();
            n->is_leaf = false;
            n->mask()  = 0;
            for (int i = 0; i < 8; ++i)
                n->kids()[i] = nullptr;
            return n;
        }

        static void destroy_node(Node* n)
        {
            if (!n)
                return;
            if (!n->is_leaf)
            {
                for (int i = 0; i < 8; ++i)
                {
                    if (n->kids()[i])
                        destroy_node(n->kids()[i]);
                }
            }
            delete n;
        }

        static size_t count_nodes(const Node* n)
        {
            if (!n)
                return 0;
            if (n->is_leaf)
                return 1;
            size_t c = 1; // internal node itself
            for (int i = 0; i < 8; ++i)
            {
                c += count_nodes(n->kids()[i]);
            }
            return c;
        }

        static FORCEINLINE i32 child_index(i32 x, i32 y, i32 z, i32 depth)
        {
            const i32 bit = 1 << (depth - 1);
            const i32 xi  = (x & bit) ? 1 : 0;
            const i32 yi  = (y & bit) ? 1 : 0;
            const i32 zi  = (z & bit) ? 1 : 0;
            return (xi) | (yi << 1) | (zi << 2);
        }

        static voxel_t get_at_node(const Node* n, i32 depth, i32 x, i32 y, i32 z)
        {
            if (!n)
                return 0;
            if (n->is_leaf || depth == 0)
                return n->leaf();
            const int   ci    = child_index(x, y, z, depth);
            const Node* child = n->kids()[ci];
            if (!child)
                return 0;
            return get_at_node(child, depth - 1, x, y, z);
        }

        // Sets voxel; returns possibly new node pointer (due to collapses)
        static Node* set_at_node(Node* n, i32 depth, i32 x, i32 y, i32 z, voxel_t v)
        {
            if (!n)
            {
                // Creating a subtree. If depth==0 we create a leaf, otherwise create
                // internal or leaf according to v
                if (depth == 0)
                    return new_leaf(v);
                if (v == 0)
                    return nullptr; // writing empty into empty: still nothing
                // create a leaf root first so we have correct default; then descend to
                // set
                n = new_leaf(0);
            }

            if (n->is_leaf)
            {
                if (depth == 0)
                {
                    n->leaf() = v;
                    return n;
                }

                // Expand leaf if we need to write a different value somewhere in subtree
                if (n->leaf() == v)
                {
                    // Setting same value under this leaf: nothing changes
                    return n;
                }

                // Expand to internal
                const voxel_t prev     = n->leaf();
                Node*         internal = new_internal();

                // If previous leaf value was non-zero, populate children with it lazily
                // on-demand. We won’t pre-create all children to keep it sparse. We'll
                // only create a child when needed.
                delete n;
                n = internal;

                // Set the prior value at the target child implicitly by creating a child
                // leaf with the previous value only when we are about to diverge. We
                // fall-through to descend and mix values correctly.
                (void)prev; // kept for potential future eager propagation
                // For correctness, we only need to ensure that non-touched areas read
                // back as `prev`. We'll encode this by storing a single optional uniform
                // child and only materialize different areas. To achieve that, we
                // temporarily set an implicit_uniform_ value on internal nodes. Simpler
                // approach: create 8 child leaves with prev if prev != 0. This is less
                // memory-efficient but is simple and correct.
                if (prev != 0)
                {
                    for (int i = 0; i < 8; ++i)
                    {
                        n->kids()[i] = new_leaf(prev);
                        n->mask() |= static_cast<u8>(1u << i);
                    }
                }
            }

            if (depth == 0)
            {
                // leaf node at correct depth
                n->leaf() = v;
                return n;
            }

            // Internal node: descend to child
            const int ci    = child_index(x, y, z, depth);
            Node*     child = n->kids()[ci];
            child           = set_at_node(child, depth - 1, x, y, z, v);

            // Update child pointer and mask
            n->kids()[ci] = child;
            if (child)
                n->mask() |= static_cast<u8>(1u << ci);
            else
                n->mask() &= static_cast<u8>(~(1u << ci));

            // Try to collapse if all children are leaves with the same value or all null
            voxel_t collapse_value{};
            bool    can_collapse = true;
            bool    any_child    = false;
            bool    first_set    = false;

            for (int i = 0; i < 8; ++i)
            {
                Node* c = n->kids()[i];
                if (!c)
                {
                    // Treat missing child as empty leaf (0)
                    if (!first_set)
                    {
                        collapse_value = 0;
                        first_set      = true;
                    }
                    else if (collapse_value != 0)
                    {
                        can_collapse = false;
                    }
                    continue;
                }
                any_child = true;
                if (!c->is_leaf)
                {
                    can_collapse = false;
                    break;
                }
                if (!first_set)
                {
                    collapse_value = c->leaf();
                    first_set      = true;
                }
                else if (collapse_value != c->leaf())
                {
                    can_collapse = false;
                    break;
                }
            }

            if (can_collapse)
            {
                // Delete all children and become a single leaf with collapse_value
                for (int i = 0; i < 8; ++i)
                {
                    if (n->kids()[i])
                    {
                        destroy_node(n->kids()[i]);
                        n->kids()[i] = nullptr;
                    }
                }
                n->is_leaf = true;
                n->leaf()  = first_set ? collapse_value : 0;
                // mask not used for leaves
            }

            // If no child exists and collapse_value is 0, the entire subtree is empty; we
            // can return nullptr
            if (n->is_leaf && n->leaf() == 0 && depth == max_depth)
            {
                // only prune root when it becomes empty leaf
                // for internal recursive nodes, we always return the node to allow
                // parent-level collapse
            }

            return n;
        }

        Node* root_{ nullptr };
    };
} // namespace v
