//
// Created by niooi on 10/13/2025.
//

#pragma once

#include <prelude.h>
#include "glm/ext/matrix_clip_space.hpp"

namespace v {
    class Camera : public Domain<Camera> {
    public:
        Camera(
            Engine& engine, f32 fov = 90, f32 aspect = 1.7777f, f32 near = 0.01f,
            f32 far = 1000.f) :
            Domain(engine), fov_(fov), aspect_(aspect), near_(near), far_(far)
        {
            attach<Rotation>();
            attach<Pos3d>();

            recalc_static_matrices();
        }

        /// Returns the view-projection matrix of the camera
        FORCEINLINE glm::mat4 matrix(){ TODO() }

        /// Get the field of view in radians
        FORCEINLINE f32 fov() const
        {
            return fov_;
        }

        /// Set the field of view (in radians) and recalculate perspective matrix
        FORCEINLINE void fov(f32 new_fov)
        {
            fov_ = new_fov;
            recalc_static_matrices();
        }

        /// Get the aspect ratio
        FORCEINLINE f32 aspect() const { return aspect_; }

        /// Set the aspect ratio and recalculate perspective matrix
        FORCEINLINE void aspect(f32 new_aspect)
        {
            aspect_ = new_aspect;
            recalc_static_matrices();
        }

        /// Get the near plane distance
        FORCEINLINE f32 near() const { return near_; }

        /// Set the near plane distance and recalculate perspective matrix
        FORCEINLINE void set_near(f32 new_near)
        {
            near_ = new_near;
            recalc_static_matrices();
        }

        /// Get the far plane distance
        FORCEINLINE f32 far() const { return far_; }

        /// Set the far plane distance and recalculate perspective matrix
        FORCEINLINE void set_far(f32 new_far)
        {
            far_ = new_far;
            recalc_static_matrices();
        }

        /// Set all perspective parameters at once
        FORCEINLINE void
        set_perspective(f32 new_fov, f32 new_aspect, f32 new_near, f32 new_far)
        {
            fov_    = new_fov;
            aspect_ = new_aspect;
            near_   = new_near;
            far_    = new_far;
            recalc_static_matrices();
        }

        /// Get the perspective matrix
        FORCEINLINE const glm::mat4& perspective() const { return perspective_; }

    private:
        f32 fov_, aspect_, near_, far_;

        glm::mat4 perspective_;

        FORCEINLINE void recalc_static_matrices()
        {
            perspective_ = glm::perspective(fov_, aspect_, near_, far_);
        }
    };
} // namespace v
