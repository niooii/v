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
        if (!root_)
            return 0;

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

    VoxelType Sparse64Tree::get_voxel(u32 x, u32 y, u32 z) const
    {
        return voxel_at(glm::vec3(x, y, z));
    }

    VoxelType Sparse64Tree::get_voxel(const glm::ivec3& pos) const
    {
        return voxel_at(glm::vec3(pos));
    }

    bool Sparse64Tree::is_node_empty(const S64Node& node) const
    {
        return node.child_mask == 0;
    }

    void Sparse64Tree::try_collapse_to_single_type(S64Node_UP& node)
    {
        if (node->type != Type::Leaf)
            return;

        if (node->child_mask == 0)
        {
            node->type = Type::Empty;
            return;
        }

        VoxelType first_type = 0;
        bool      found_first = false;

        for (u32 i = 0; i < 64; ++i)
        {
            if (node->child_mask & (1ull << i))
            {
                if (!found_first)
                {
                    first_type  = node->voxels[i];
                    found_first = true;
                }
                else if (node->voxels[i] != first_type)
                {
                    return;
                }
            }
        }

        if (found_first && node->child_mask == ~0ull)
        {
            fill_node(node, first_type);
        }
    }

    bool Sparse64Tree::should_collapse_regular(S64Node_UP& node)
    {
        return node->type == Type::Regular && node->child_mask == 0;
    }

    void Sparse64Tree::set_voxel(u32 x, u32 y, u32 z, VoxelType type)
    {
        glm::uvec3 u_pos(x, y, z);
        u8         shift_amt = init_shift_amt();

        if (!root_)
        {
            if (type == 0)
                return;
            root_ = std::make_unique<S64Node>();
            root_->type = Type::Regular;
            root_->children.resize(64);
        }

        S64Node_P                curr       = root_.get();
        std::vector<S64Node_P>   path;
        std::vector<u8>          indices;

        while (1)
        {
            u8 idx = static_cast<u8>(curr->get_idx(
                u_pos.x >> shift_amt, u_pos.y >> shift_amt, u_pos.z >> shift_amt));

            if (shift_amt == 0)
            {
                switch (curr->type)
                {
                case Type::SingleTypeLeaf:
                {
                    VoxelType existing = curr->voxels[0];
                    if (type == existing)
                        return;

                    if (type == 0)
                    {
                        curr->type       = Type::Leaf;
                        curr->voxels.resize(64, existing);
                        curr->child_mask = ~0ull;
                        curr->voxels[idx] = 0;
                        curr->child_mask &= ~(1ull << idx);
                    }
                    else
                    {
                        curr->voxels.resize(64, existing);
                        curr->voxels[idx] = type;
                        curr->type        = Type::Leaf;
                        curr->child_mask  = ~0ull;
                    }
                    break;
                }
                case Type::Leaf:
                {
                    VoxelType existing = (curr->child_mask & (1ull << idx)) ? curr->voxels[idx] : 0;
                    if (existing == type)
                        return;

                    if (type == 0)
                    {
                        curr->child_mask &= ~(1ull << idx);
                        curr->voxels[idx] = 0;
                    }
                    else
                    {
                        curr->child_mask |= (1ull << idx);
                        curr->voxels[idx] = type;
                    }
                    break;
                }
                default:
                {
                    if (type == 0)
                        return;

                    curr->type = Type::Leaf;
                    curr->voxels.resize(64, 0);
                    curr->voxels[idx] = type;
                    curr->child_mask  = (1ull << idx);
                    break;
                }
                }

                break;
            }

            path.push_back(curr);
            indices.push_back(idx);

            if (curr->type == Type::SingleTypeLeaf)
            {
                VoxelType fill_type = curr->voxels[0];
                curr->type          = Type::Regular;
                curr->children.resize(64);
                curr->child_mask = 0;

                S64Node_UP new_child = std::make_unique<S64Node>();
                new_child->parent    = curr;
                new_child->type      = Type::SingleTypeLeaf;
                new_child->voxels    = { fill_type };
                new_child->child_mask = 0b1;

                curr->children[idx]  = std::move(new_child);
                curr->child_mask |= (1ull << idx);
            }
            else if (!(curr->child_mask & (1ull << idx)))
            {
                if (type == 0)
                    return;

                S64Node_UP new_child = std::make_unique<S64Node>();
                new_child->parent    = curr;
                new_child->type      = Type::Regular;
                new_child->children.resize(64);

                curr->children[idx]  = std::move(new_child);
                curr->child_mask |= (1ull << idx);
            }

            curr = curr->children[idx].get();
            to_local_coords(u_pos, shift_amt);
            shift_amt -= 2;
        }

        for (i32 i = static_cast<i32>(path.size()) - 1; i >= 0; --i)
        {
            S64Node_P parent     = path[i];
            u8        child_idx  = indices[i];
            S64Node_UP& child    = parent->children[child_idx];

            if (child->type == Type::Leaf)
            {
                try_collapse_to_single_type(child);
            }

            if (is_node_empty(*child) || child->type == Type::Empty)
            {
                child.reset();
                parent->child_mask &= ~(1ull << child_idx);
            }
        }

        if (root_->type == Type::Regular && root_->child_mask == 0)
        {
            root_.reset();
        }
        else if (root_->type == Type::Leaf)
        {
            try_collapse_to_single_type(root_);
        }

        dirty_ = true;
    }

    void Sparse64Tree::set_voxel(const glm::ivec3& pos, VoxelType type)
    {
        set_voxel(pos.x, pos.y, pos.z, type);
    }

    void Sparse64Tree::fill_aabb(const AABB& region, VoxelType type)
    {
        AABB clipped(
            glm::max(region.min, bounds_.min),
            glm::min(region.max, bounds_.max));

        if (clipped.min.x >= clipped.max.x || clipped.min.y >= clipped.max.y || clipped.min.z >= clipped.max.z)
            return;

        glm::ivec3 start = glm::ivec3(glm::floor(clipped.min));
        glm::ivec3 end   = glm::ivec3(glm::ceil(clipped.max));

        for (i32 x = start.x; x < end.x; ++x)
            for (i32 y = start.y; y < end.y; ++y)
                for (i32 z = start.z; z < end.z; ++z)
                    set_voxel(x, y, z, type);
    }

    void Sparse64Tree::fill_sphere(const glm::vec3& center, f32 radius, VoxelType type)
    {
        AABB sphere_bounds(
            center - glm::vec3(radius),
            center + glm::vec3(radius));

        AABB clipped(
            glm::max(sphere_bounds.min, bounds_.min),
            glm::min(sphere_bounds.max, bounds_.max));

        if (clipped.min.x >= clipped.max.x || clipped.min.y >= clipped.max.y || clipped.min.z >= clipped.max.z)
            return;

        glm::ivec3 start    = glm::ivec3(glm::floor(clipped.min));
        glm::ivec3 end      = glm::ivec3(glm::ceil(clipped.max));
        f32        r_sq     = radius * radius;

        for (i32 x = start.x; x < end.x; ++x)
            for (i32 y = start.y; y < end.y; ++y)
                for (i32 z = start.z; z < end.z; ++z)
                {
                    glm::vec3 voxel_center = glm::vec3(x, y, z) + glm::vec3(0.5f);
                    glm::vec3 diff         = voxel_center - center;
                    if (glm::dot(diff, diff) <= r_sq)
                        set_voxel(x, y, z, type);
                }
    }

    void Sparse64Tree::fill_cylinder(const glm::vec3& p0, const glm::vec3& p1, f32 radius, VoxelType type)
    {
        glm::vec3 axis   = p1 - p0;
        f32       length = glm::length(axis);
        if (length < 1e-6f)
            return;

        axis /= length;

        AABB cyl_bounds(
            glm::min(p0, p1) - glm::vec3(radius),
            glm::max(p0, p1) + glm::vec3(radius));

        AABB clipped(
            glm::max(cyl_bounds.min, bounds_.min),
            glm::min(cyl_bounds.max, bounds_.max));

        if (clipped.min.x >= clipped.max.x || clipped.min.y >= clipped.max.y || clipped.min.z >= clipped.max.z)
            return;

        glm::ivec3 start = glm::ivec3(glm::floor(clipped.min));
        glm::ivec3 end   = glm::ivec3(glm::ceil(clipped.max));
        f32        r_sq  = radius * radius;

        for (i32 x = start.x; x < end.x; ++x)
            for (i32 y = start.y; y < end.y; ++y)
                for (i32 z = start.z; z < end.z; ++z)
                {
                    glm::vec3 voxel_center = glm::vec3(x, y, z) + glm::vec3(0.5f);
                    glm::vec3 to_voxel     = voxel_center - p0;
                    f32       t            = glm::dot(to_voxel, axis);

                    if (t >= 0.0f && t <= length)
                    {
                        glm::vec3 closest = p0 + axis * t;
                        glm::vec3 diff    = voxel_center - closest;
                        if (glm::dot(diff, diff) <= r_sq)
                            set_voxel(x, y, z, type);
                    }
                }
    }
} // namespace v
