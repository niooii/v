//
// Created by niooi on 10/17/2025.
//

#pragma once

#include <engine/camera.h>
#include "defs.h"
#include "engine/contexts/window/window.h"

// An implementation of a 'free cam' camera (or editor camera/dev camera idk)
// It just comes with default keybinds for moving and looking around, for convenience

namespace v {
    class DevCamera : public SDomain<DevCamera> {
    public:
        DevCamera(Engine& engine) : SDomain(engine), camera_(attach<Camera>())
        {
            engine.on_tick.connect(
                {}, {}, "dev_cam_upd",
                [&]
                {
                    auto window = engine.get_ctx<WindowContext>()->get_window();

                    const f32 move_speed       = 1.5f * engine_.delta_time();
                    const f32 look_sensitivity = 0.02f;

                    auto& pos = camera_.get<Pos3d>().val;

                    if (window->is_key_down(Key::W))
                        pos += camera_.forward() * move_speed;
                    if (window->is_key_down(Key::S))
                        pos -= camera_.forward() * move_speed;
                    if (window->is_key_down(Key::A))
                        pos -= camera_.right() * move_speed;
                    if (window->is_key_down(Key::D))
                        pos += camera_.right() * move_speed;
                    if (window->is_key_down(Key::Q))
                        pos += camera_.up() * move_speed;
                    if (window->is_key_down(Key::E))
                        pos -= camera_.up() * move_speed;

                    glm::vec2 delta = glm::vec2(window->get_mouse_delta());

                    camera_.add_yaw(-delta.x * look_sensitivity);
                    camera_.add_pitch(delta.y * look_sensitivity);
                });
        }

        ~DevCamera() { engine_.on_tick.disconnect("dev_cam_upd"); }

        FORCEINLINE Camera& camera() { return camera_; };

    private:
        Camera& camera_;
    };
} // namespace v
