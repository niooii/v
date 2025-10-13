//
// Created by niooi on 7/29/2025.
//

#pragma once

#include <defs.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

/// A bunch of commonly used component definitions
/// TODO! before this gets too messy, prefer
/// strong typedefs instead?

struct Pos2d {
    glm::vec2 val;
};

struct Size2d {
    glm::vec2 val;
};

struct Pos3d {
    glm::vec3 val;

    /// Returns the homogenous translation matrix
    FORCEINLINE glm::mat4 matrix() { return glm::translate({}, val); }
};

struct Size3d {
    glm::vec3 val;

    FORCEINLINE glm::mat4 matrix() { return glm::scale({}, val); }
};

struct Rotation {
    glm::quat val;

    /// Returns the rotation matrix
    FORCEINLINE glm::mat4 matrix() { return glm::toMat4(val); }
};
