# Voxel Storage

This guide explains V Engine's voxel storage system, which uses a Sparse64Tree octree structure for efficient storage and manipulation of voxel data.

## Overview

V Engine stores voxel data using a specialized sparse octree called **Sparse64Tree**. This data structure is optimized for:

- **Memory Efficiency**: Only stores non-empty voxels, implicit air representation
- **GPU Compatibility**: Flattened representation suitable for GPU consumption
- **Fast Operations**: Hierarchical structure enables efficient bulk operations
- **Scalability**: Handles large voxel worlds with sparse distribution

## Sparse64Tree Architecture

### Tree Structure

The Sparse64Tree is a 4³ octree (64 children per node) that organizes voxel data hierarchically:

```
Level 0: Root (covers entire world)
Level 1: 64 child nodes (4x4x4)
Level 2: 4,096 child nodes (16x16x16)
Level 3: 262,144 child nodes (64x64x64)
Level 4: 16,777,216 leaf nodes (256x256x256)
```

Each node at level N represents a 4^N voxel region.

### Node Types

```cpp
// Internal node - has children
struct S64Node {
    std::vector<S64Node*> children;  // 64 child pointers (allocated on demand)
    uint32_t depth;                   // Tree depth
    bool is_leaf;                     // True for leaf nodes
};

// Leaf node - contains voxel data
struct S64LeafNode : public S64Node {
    VoxelData voxels[64];             // 4x4x4 = 64 voxels per leaf
};
```

### Memory Layout

```cpp
class Sparse64Tree {
private:
    std::unique_ptr<S64Node> root_;    // Root node
    uint32_t max_depth_;              // Maximum tree depth
    std::vector<S64Node*> node_pool_; // Memory pool for nodes

public:
    // Voxel operations
    VoxelData get_voxel(int32_t x, int32_t y, int32_t z) const;
    void set_voxel(int32_t x, int32_t y, int32_t z, VoxelData data);
    bool is_air(int32_t x, int32_t y, int32_t z) const;

    // Bulk operations
    void fill_region(AABB region, VoxelData data);
    void clear_region(AABB region);
    void copy_region(AABB src, AABB dst);

    // GPU integration
    std::vector<GS64Node> get_gpu_representation() const;
    void mark_dirty() { dirty_ = true; }
    bool is_dirty() const { return dirty_; }
};
```

## Basic Usage

### Creating a Voxel Storage

```cpp
#include <vox/store/64tree.h>

class TerrainDomain : public v::Domain<TerrainDomain> {
public:
    TerrainDomain(v::Engine& engine) : v::Domain(engine, "Terrain") {
        // Create voxel storage for a chunk
        voxel_storage_ = std::make_unique<Sparse64Tree>(
            /* world_size */ {256, 256, 256},
            /* max_depth */ 4
        );
    }

private:
    std::unique_ptr<Sparse64Tree> voxel_storage_;
};
```

### Setting and Getting Voxels

```cpp
void TerrainDomain::generate_terrain() {
    // Set individual voxels
    for (int x = 0; x < 100; ++x) {
        for (int z = 0; z < 100; ++z) {
            // Simple height function
            int height = 50 + sin(x * 0.1f) * 10 + cos(z * 0.1f) * 5;

            for (int y = 0; y < height; ++y) {
                VoxelData voxel{
                    .type = VoxelType::Stone,
                    .density = 1.0f,
                    .color = {0.7f, 0.7f, 0.7f}
                };
                voxel_storage_->set_voxel(x, y, z, voxel);
            }
        }
    }
}

VoxelData TerrainDomain::get_voxel_at(int32_t x, int32_t y, int32_t z) {
    return voxel_storage_->get_voxel(x, y, z);
}
```

### Efficient Queries

```cpp
bool TerrainDomain::is_solid(int32_t x, int32_t y, int32_t z) {
    // Fast air check - no allocation for empty space
    return !voxel_storage_->is_air(x, y, z);
}

std::vector<VoxelData> TerrainDomain::get_region(AABB region) {
    std::vector<VoxelData> voxels;
    voxels.reserve(region.volume());

    for (int32_t x = region.min.x; x < region.max.x; ++x) {
        for (int32_t y = region.min.y; y < region.max.y; ++y) {
            for (int32_t z = region.min.z; z < region.max.z; ++z) {
                if (!voxel_storage_->is_air(x, y, z)) {
                    voxels.push_back(voxel_storage_->get_voxel(x, y, z));
                }
            }
        }
    }

    return voxels;
}
```

## Bulk Operations

### Filling Regions

```cpp
void TerrainDomain::create_cube(glm::ivec3 position, int size, VoxelData material) {
    AABB region{
        .min = position,
        .max = position + glm::ivec3(size)
    };

    voxel_storage_->fill_region(region, material);
}

void TerrainDomain::create_sphere(glm::ivec3 center, int radius, VoxelData material) {
    // Use tree traversal for efficient sphere filling
    std::function<void(S64Node*, glm::ivec3, int)> fill_node =
        [&](S64Node* node, glm::ivec3 node_pos, int node_size) {

        // Check if sphere intersects this node
        glm::vec3 closest_point = glm::clamp(glm::vec3(center),
                                            glm::vec3(node_pos),
                                            glm::vec3(node_pos + node_size));
        float distance = glm::length(glm::vec3(center) - closest_point);

        if (distance > radius + glm::length(glm::vec3(node_size)) * 0.866f) {
            // Sphere doesn't intersect this node - skip
            return;
        }

        if (node->is_leaf) {
            // Fill leaf voxels
            auto* leaf = static_cast<S64LeafNode*>(node);
            for (int i = 0; i < 64; ++i) {
                glm::ivec3 voxel_pos = node_pos + decode_morton3(i);
                if (glm::length(glm::vec3(center - voxel_pos)) <= radius) {
                    leaf->voxels[i] = material;
                }
            }
        } else {
            // Recurse to children
            for (int i = 0; i < 64; ++i) {
                if (node->children[i]) {
                    glm::ivec3 child_pos = node_pos + decode_morton3(i) * (node_size / 4);
                    fill_node(node->children[i], child_pos, node_size / 4);
                }
            }
        }
    };

    fill_node(voxel_storage_->get_root(), {0, 0, 0}, voxel_storage_->get_world_size());
}
```

### Copy and Modify Operations

```cpp
void TerrainDomain::copy_region(AABB src, AABB dst) {
    voxel_storage_->copy_region(src, dst);
}

void TerrainDomain::carve_tunnel(glm::vec3 start, glm::vec3 end, float radius) {
    glm::vec3 direction = glm::normalize(end - start);
    float length = glm::length(end - start);

    for (float t = 0; t < length; t += 0.5f) {
        glm::vec3 pos = start + direction * t;
        glm::ivec3 voxel_pos = glm::floor(pos);

        // Clear voxels in sphere around position
        create_sphere(voxel_pos, static_cast<int>(radius),
                     VoxelData{.type = VoxelType::Air});
    }
}
```

## GPU Integration

### GPU Representation

The tree is flattened for GPU consumption:

```cpp
struct GS64Node {
    uint32_t children_start;   // Index in children array (0 if no children)
    uint32_t leaf_data;        // Packed voxel data (0 if not leaf)
    uint8_t depth;             // Tree depth
    uint8_t padding[3];
};

std::vector<GS64Node> TerrainDomain::get_gpu_nodes() const {
    if (!voxel_storage_->is_dirty()) {
        return cached_gpu_nodes_;
    }

    // Flatten tree structure
    std::vector<GS64Node> gpu_nodes;
    std::vector<uint32_t> children_indices;

    std::function<void(S64Node*)> flatten_node = [&](S64Node* node) {
        uint32_t node_index = gpu_nodes.size();

        GS64Node gpu_node{};
        gpu_node.depth = node->depth;

        if (node->is_leaf) {
            // Pack leaf voxel data
            auto* leaf = static_cast<S64LeafNode*>(node);
            gpu_node.leaf_data = pack_voxel_data(leaf->voxels);
        } else {
            // Reserve space for children indices
            gpu_node.children_start = children_indices.size() + 1; // +1 for 0 = no children
            children_indices.push_back(0); // Placeholder for children count

            uint32_t children_count = 0;
            for (int i = 0; i < 64; ++i) {
                if (node->children[i]) {
                    children_indices.push_back(gpu_nodes.size() + 1); // Child will be next
                    flatten_node(node->children[i]);
                    children_count++;
                }
            }

            // Update children count
            children_indices[gpu_node.children_start - 1] = children_count;
        }

        gpu_nodes.push_back(gpu_node);
    };

    flatten_node(voxel_storage_->get_root());

    cached_gpu_nodes_ = gpu_nodes;
    voxel_storage_->clear_dirty();

    return gpu_nodes;
}
```

### Rendering Integration

```cpp
class VoxelRenderDomain : public v::RenderDomain<VoxelRenderDomain> {
public:
    VoxelRenderDomain(v::Engine& engine) : v::RenderDomain(engine, "VoxelRender") {
        auto& daxa = render_ctx_->daxa_resources();

        // Create GPU buffers for voxel data
        voxel_buffer_ = daxa.device.create_buffer({
            .size = sizeof(GS64Node) * max_voxel_nodes_,
            .usage = daxa::BufferUsageFlagBits::STORAGE_BUFFER |
                     daxa::BufferUsageFlagBits::TRANSFER_DST,
            .name = "voxel_nodes_buffer",
        });

        // Create compute pipeline for voxel rendering
        voxel_pipeline_ = daxa.pipeline_manager.add_compute_pipeline2({
            .shader_info = daxa::ShaderCompileInfo2{
                .source = daxa::ShaderFile{"voxel_render.glsl"},
                .defines = {{"COMPUTE_SHADER", "1"}},
            },
            .name = "voxel_render_pipeline",
        }).value();
    }

    void add_render_tasks(daxa::TaskGraph& graph) override {
        auto* terrain = engine_.try_get_domain<TerrainDomain>();
        if (!terrain) return;

        graph.add_task(daxa::Task::Compute("voxel_render")
            .reads(voxel_buffer_, daxa::TaskBufferAccess::SHADER_READ)
            .writes(output_image_, daxa::TaskImageAccess::SHADER_WRITE)
            .executes([this, terrain](daxa::TaskInterface ti) {
                // Update GPU buffer if terrain changed
                if (terrain->is_voxel_data_dirty()) {
                    upload_voxel_data(terrain->get_gpu_nodes());
                    terrain->clear_dirty_flag();
                }

                // Dispatch compute shader
                auto pipeline = ti.get_pipeline(voxel_pipeline_);
                ti.recorder.bind_pipeline(pipeline);

                // Set uniforms and dispatch
                struct PushConstants {
                    glm::mat4 view_proj;
                    glm::vec3 camera_pos;
                    uint32_t node_count;
                } push_consts{
                    .view_proj = view_projection_matrix_,
                    .camera_pos = camera_position_,
                    .node_count = static_cast<uint32_t>(node_count_)
                };

                ti.recorder.push_constants(push_consts);
                ti.recorder.dispatch({
                    .x = (screen_size_.x + 15) / 16,
                    .y = (screen_size_.y + 15) / 16,
                    .z = 1
                });
            })
        );
    }

private:
    void upload_voxel_data(const std::vector<GS64Node>& nodes) {
        auto& daxa = render_ctx_->daxa_resources();

        auto* buffer_ptr = daxa.device.map_buffer_as<GS64Node>(
            voxel_buffer_.get_state().buffers[0]);

        std::memcpy(buffer_ptr, nodes.data(),
                   nodes.size() * sizeof(GS64Node));

        daxa.device.unmap_buffer(voxel_buffer_.get_state().buffers[0]);
        node_count_ = nodes.size();
    }

    daxa::BufferId voxel_buffer_;
    std::shared_ptr<daxa::ComputePipeline> voxel_pipeline_;
    size_t max_voxel_nodes_ = 1000000;
    size_t node_count_ = 0;
    glm::mat4 view_projection_matrix_;
    glm::vec3 camera_position_;
    glm::uvec2 screen_size_;
};
```

## Performance Considerations

### Memory Efficiency

```cpp
class VoxelMemoryManager {
public:
    // Memory pool for efficient node allocation
    class NodePool {
    private:
        std::vector<std::unique_ptr<S64Node[]>> pools_;
        std::queue<S64Node*> available_nodes_;
        size_t pool_size_ = 1024;

    public:
        S64Node* allocate() {
            if (available_nodes_.empty()) {
                allocate_new_pool();
            }

            S64Node* node = available_nodes_.front();
            available_nodes_.pop();
            return node;
        }

        void deallocate(S64Node* node) {
            // Reset node and return to pool
            memset(node, 0, sizeof(S64Node));
            available_nodes_.push(node);
        }

    private:
        void allocate_new_pool() {
            auto pool = std::make_unique<S64Node[]>(pool_size_);
            for (size_t i = 0; i < pool_size_; ++i) {
                available_nodes_.push(&pool[i]);
            }
            pools_.push_back(std::move(pool));
        }
    };
};
```

### Lazy Loading

```cpp
class ChunkedVoxelWorld {
public:
    VoxelData get_voxel(glm::ivec3 world_pos) {
        glm::ivec3 chunk_pos = world_to_chunk(world_pos);

        // Load chunk on demand
        if (!is_chunk_loaded(chunk_pos)) {
            load_chunk(chunk_pos);
        }

        auto* chunk = get_chunk(chunk_pos);
        glm::ivec3 local_pos = world_pos - chunk_pos * chunk_size_;

        return chunk->get_voxel(local_pos);
    }

private:
    glm::ivec3 world_to_chunk(glm::ivec3 world_pos) {
        return glm::floor(glm::vec3(world_pos) / glm::vec3(chunk_size_));
    }

    void load_chunk(glm::ivec3 chunk_pos) {
        // Generate or load chunk data
        auto chunk = std::make_unique<Sparse64Tree>(chunk_size_, max_depth_);

        // Generate terrain for this chunk
        generate_terrain_for_chunk(chunk.get(), chunk_pos);

        chunks_[chunk_pos] = std::move(chunk);
    }

    static constexpr int chunk_size_ = 256;
    std::unordered_map<glm::ivec3, std::unique_ptr<Sparse64Tree>> chunks_;
};
```

### Spatial Queries

```cpp
class VoxelSpatialIndex {
public:
    // Efficient ray-voxel intersection
    std::vector<glm::ivec3> raycast(glm::vec3 start, glm::vec3 direction, float max_distance) {
        std::vector<glm::ivec3> hit_voxels;

        // Use DDA algorithm for efficient traversal
        glm::vec3 current = start;
        float distance = 0.0f;

        while (distance < max_distance) {
            glm::ivec3 voxel_pos = glm::floor(current);

            if (!is_air(voxel_pos.x, voxel_pos.y, voxel_pos.z)) {
                hit_voxels.push_back(voxel_pos);

                // Stop at first hit for solid raycast
                break;
            }

            // Step to next voxel
            glm::vec3 step_sizes = 1.0f / glm::abs(direction);
            glm::vec3 next_boundaries = (glm::floor(current) + glm::sign(direction) - current) / direction;

            float min_step = std::min({next_boundaries.x, next_boundaries.y, next_boundaries.z});
            current += direction * min_step;
            distance += min_step;
        }

        return hit_voxels;
    }

    // Efficient sphere intersection
    std::vector<VoxelData> get_voxels_in_sphere(glm::vec3 center, float radius) {
        std::vector<VoxelData> voxels;

        // Use hierarchical culling
        std::function<void(S64Node*, glm::ivec3, int)> query_node =
            [&](S64Node* node, glm::ivec3 node_pos, int node_size) {

            // Check if sphere intersects node bounds
            glm::vec3 closest = glm::clamp(center,
                                          glm::vec3(node_pos),
                                          glm::vec3(node_pos + node_size));
            float distance = glm::length(center - closest);

            if (distance > radius + glm::length(glm::vec3(node_size)) * 0.866f) {
                return; // No intersection
            }

            if (node->is_leaf) {
                // Check individual voxels
                auto* leaf = static_cast<S64LeafNode*>(node);
                for (int i = 0; i < 64; ++i) {
                    glm::ivec3 voxel_pos = node_pos + decode_morton3(i);
                    if (glm::length(center - glm::vec3(voxel_pos)) <= radius) {
                        voxels.push_back(leaf->voxels[i]);
                    }
                }
            } else {
                // Recurse to children
                for (int i = 0; i < 64; ++i) {
                    if (node->children[i]) {
                        glm::ivec3 child_pos = node_pos + decode_morton3(i) * (node_size / 4);
                        query_node(node->children[i], child_pos, node_size / 4);
                    }
                }
            }
        };

        query_node(voxel_storage_->get_root(), {0, 0, 0}, voxel_storage_->get_world_size());
        return voxels;
    }
};
```

## Best Practices

1. **Use Bulk Operations**: Fill regions rather than individual voxels when possible
2. **Lazy Loading**: Load voxel data on demand for large worlds
3. **Dirty Tracking**: Mark regions as dirty to avoid unnecessary GPU updates
4. **Memory Pooling**: Use object pools for frequent node allocation/deallocation
5. **Spatial Culling**: Use hierarchical traversal for spatial queries
6. **GPU Batching**: Update GPU buffers in batches rather than per-voxel

## Limitations and Considerations

1. **Memory Usage**: While sparse, very dense worlds can still use significant memory
2. **Tree Depth**: Deeper trees provide better granularity but increase traversal cost
3. **GPU Memory**: Large voxel worlds may exceed GPU memory limits
4. **Update Performance**: Real-time modifications to large regions can be expensive
5. **Serialization**: Tree structure requires careful serialization for persistence

The Sparse64Tree provides an excellent balance between memory efficiency and performance for voxel-based applications, particularly suited for games with large, sparse voxel worlds.