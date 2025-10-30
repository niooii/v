//
// Created by niooi on 9/24/2025.
//

#include <engine/engine.h>
#include <world/world.h>

namespace v {

    static FORCEINLINE i32 div_floor_i32(i32 a, i32 b)
    {
        // b assumed > 0
        if (a >= 0)
            return a / b;
        return -static_cast<i32>((-static_cast<i64>(a) + (b - 1)) / b);
    }

    static FORCEINLINE i32 mod_floor_i32(i32 a, i32 b)
    {
        // b assumed > 0
        i32 r = a % b;
        if (r < 0)
            r += b;
        return r;
    }

    std::pair<ChunkPos, VoxelPos> WorldDomain::world_to_chunk(WorldPos wp)
    {
        const i32 cs = k_chunk_size;
        ChunkPos  cp{
            div_floor_i32(wp.x, cs),
            div_floor_i32(wp.y, cs),
            div_floor_i32(wp.z, cs),
        };
        VoxelPos lp{ mod_floor_i32(wp.x, cs), mod_floor_i32(wp.y, cs),
                     mod_floor_i32(wp.z, cs) };
        return { cp, lp };
    }

    ChunkDomain* WorldDomain::try_get_chunk(const ChunkPos& cp)
    {
        auto it = chunks_.find(cp);
        if (it == chunks_.end())
            return nullptr;
        return it->second;
    }

    const ChunkDomain* WorldDomain::try_get_chunk(const ChunkPos& cp) const
    {
        auto it = chunks_.find(cp);
        if (it == chunks_.end())
            return nullptr;
        return it->second;
    }

    ChunkDomain& WorldDomain::get_or_create_chunk(const ChunkPos& cp)
    {
        auto it = chunks_.find(cp);
        if (it != chunks_.end())
            return *it->second;

        std::string name = "Chunk(" + std::to_string(cp.x) + "," + std::to_string(cp.y) +
            "," + std::to_string(cp.z) + ")";
        auto& chunk = engine().add_domain<ChunkDomain>(cp, name);
        chunks_.emplace(cp, &chunk);
        return chunk;
    }

    bool WorldDomain::remove_chunk(const ChunkPos& cp)
    {
        auto it = chunks_.find(cp);
        if (it == chunks_.end())
            return false;
        if (it->second)
        {
            entt::entity id = it->second->entity();
            engine().post_tick(
                [this, id]()
                {
                    if (engine().registry().valid(id))
                        engine().registry().destroy(id);
                });
        }
        chunks_.erase(it);
        return true;
    }

    bool WorldDomain::has_chunk(const ChunkPos& cp) const { return chunks_.contains(cp); }

    u16 WorldDomain::get_voxel(WorldPos wp) const
    {
        auto [cp, lp] = world_to_chunk(wp);
        if (auto it = chunks_.find(cp); it != chunks_.end())
        {
            return it->second->get(lp);
        }
        return 0;
    }

    void WorldDomain::set_voxel(WorldPos wp, u16 value)
    {
        auto [cp, lp] = world_to_chunk(wp);
        auto& chunk   = const_cast<WorldDomain*>(this)->get_or_create_chunk(cp);
        chunk.set(lp, value);
    }
} // namespace v
