//
// Created by niooi on 10/6/2025.
//

#pragma once

// Implementation of a 4^3 tree (each node has 64 children)
// Unlike many implementations of SVTs where the 'depth' of the tree corresponds to how
// small subdivisions (the voxel size) gets, this implementation is based around the model
// of depth as the maximum volume of the tree. The voxel size is fixed to <1, 1, 1>, and
// depth will define how 'large' of a volume the tree occupies in world space. I've found
// this personally to be a nicer mental model. Too bad the rest of the engine will have to
// think so too.

#include <defs.h>
#include <math.h>
#include <vox/aabb.h>

namespace v {
    struct GS64Node {};

    struct S64Node {
        std::unique_ptr<S64Node> children[64];
        S64Node*                 parent     = nullptr;
        u64                       child_mask = 0;
        bool                      is_leaf    = false;
        std::vector<uint8_t>      voxels;
    };

    typedef std::unique_ptr<S64Node> S64Node_P;

    class Sparse64Tree {
    public:
        explicit Sparse64Tree(u8 depth) :
            bounds_(glm::vec3(0), glm::vec3(v::pow(4.f, static_cast<f32>(depth)))),
            // allocates 1/512th of the maximum space upfront
            g_nodes_(static_cast<u32>(v::pow(8.f, depth)))
        {}

        /// Constructs the smallest 64Tree that can contain the bounding box.
        /// Translation of the box does not matter.
        /// TODO! bunch of warnings and stuff
        explicit Sparse64Tree(const AABB& must_contain) :
            Sparse64Tree(
                static_cast<u8>(v::ceil_log(
                    v::max_component(must_contain.max - must_contain.min), 4.0)))
        {}

        /// Returns the bounding box that this tree occupies in it's local object space.
        /// The box will always be properly oriented.
        /// One vertex (min) will always be the origin, such that the other vertex (max)
        /// will always be in the positive octant.
        FORCEINLINE const AABB& bounding_box() const { return bounds_; };

        /// Flattens the tree into an array of GPU friendly nodes.
        // TODO! should maybe move into different place? so 64tree only worries about cpu
        // side storage? idk
        void                          flatten() const;
        const std::vector<GS64Node>& gpu_nodes() { return g_nodes_; };


        /// Destroys the contents of the entire tree
        FORCEINLINE void clear() { clear_node(root_); dirty_ = true; }

    private:
        S64Node_P root_{ nullptr };
        AABB       bounds_;

        /// whether the flat gpu node buffer needs rebuilding
        bool                   dirty_{};
        std::vector<GS64Node> g_nodes_;

        /// recursively destroys nodes
        void clear_node(S64Node_P& node);
    };
} // namespace v
