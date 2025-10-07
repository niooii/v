//
// Created by niooi on 10/6/2025.
//

#pragma once

#include <defs.h>
#include <glm/glm.hpp>

struct AABB {
public:
    AABB(const glm::vec3& min, const glm::vec3& max) : min(min), max(max) {}

    /// Translates the box such that min = <0, 0, 0>. Max is translated accordingly.
    FORCEINLINE AABB& center_min()
    {
        max -= min;
        min = glm::vec3(0);

        return *this;
    }

    /// Translates the box such that min = <0, 0, 0>. Max is translated accordingly.
    FORCEINLINE AABB centered_min()
    {
        AABB o = *this;
        o.center_min();
        return o;
    }

    /// Transforms the box such that max >= min (all components of min become less than
    /// all components of max), preserving volume. The min vertex is not changed, only the
    /// max vertex is modified.
    FORCEINLINE AABB& reorient()
    {
        glm::vec3 diff = max - min;
        if (diff.x < 0)
            max.x += 2 * diff.x;
        if (diff.y < 0)
            max.y += 2 * diff.y;
        if (diff.z < 0)
            max.z += 2 * diff.z;

        return *this;
    }

    /// Transforms the box such that max >= min (all components of min become less than
    /// all components of max), preserving volume. The min vertex is not changed, only the
    /// max vertex is modified.
    FORCEINLINE AABB reoriented()
    {
        AABB o = *this;
        o.reorient();
        return o;
    }

    /// Translates the AABB by the given offset vector.
    FORCEINLINE AABB& translate(const glm::vec3& offset)
    {
        min += offset;
        max += offset;
        return *this;
    }

    /// Translates the AABB by the given offset vector.
    FORCEINLINE AABB translated(const glm::vec3& offset) const
    {
        AABB o = *this;
        o.translate(offset);
        return o;
    }

    glm::vec3 min;
    glm::vec3 max;
};
