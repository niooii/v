//
// Created by niooi on 10/13/2025.
//

#pragma once

#include <prelude.h>

namespace v {
    class Camera : public Domain<Camera> {
    public:
        Camera(
            Engine& engine, f32 fov = 90, f32 aspect = 1.7777f, f32 near = 0.01f,
            f32 far = 1000.f) : Domain(engine)
        {
            attach<Rotation>();
            attach<Pos3d>();
        }

        /// Returns the transform matrix of the camera
        FORCEINLINE glm::mat4 matrix() { TODO() }

    private:
        f32 fov, aspect, near, far;
    };
} // namespace v
