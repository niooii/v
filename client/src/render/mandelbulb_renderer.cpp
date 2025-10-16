//
// Created by niooi on 10/15/2025.
//

#include <engine/camera.h>
#include <engine/contexts/render/ctx.h>
#include <engine/contexts/render/init_vk.h>
#include <engine/contexts/window/window.h>
#include <engine/engine.h>
#include <render/mandelbulb_renderer.h>

namespace v {
    MandelbulbRenderer::MandelbulbRenderer(Engine& engine) :
        RenderDomain(engine, "Mandelbulb")
    {
        auto& daxa = render_ctx_->daxa_resources();

        compute_pipeline_ = daxa.pipeline_manager
                                .add_compute_pipeline2(
                                    {
                                        .source = daxa::ShaderFile{ "mandelbulb.glsl" },
                                        .name   = "mandelbulb_compute",
                                    })
                                .value();

        camera_                   = engine_.add_domain<Camera>();
        camera_->get<Pos3d>().val = glm::vec3(0.0f, 0.0f, 5.0f);

        window_ = engine_.get_domain<Window>();

        LOG_INFO("MandelbulbRenderer initialized");
    }

    MandelbulbRenderer::~MandelbulbRenderer()
    {
        if (render_ctx_)
        {
            auto& daxa = render_ctx_->daxa_resources();
            daxa.device.wait_idle();
            if (daxa.device.is_id_valid(render_image_))
                daxa.device.destroy_image(render_image_);
        }
    }

    void MandelbulbRenderer::add_render_tasks(daxa::TaskGraph& graph)
    {
        auto& swapchain_image = render_ctx_->swapchain_image();
        auto& daxa            = render_ctx_->daxa_resources();

        // Get current swapchain extent
        auto swapchain_extent = render_ctx_->get_swapchain_extent();

        // Recreate image if extent changed
        bool needs_recreate =
            (swapchain_extent.x != last_extent_.x ||
             swapchain_extent.y != last_extent_.y);

        if (needs_recreate || !daxa.device.is_id_valid(render_image_))
        {
            // Destroy old image if it exists
            if (daxa.device.is_id_valid(render_image_))
            {
                daxa.device.wait_idle();
                daxa.device.destroy_image(render_image_);
            }

            // square aspect
            u32 size = std::min(swapchain_extent.x, swapchain_extent.y);

            // Create new storage image
            render_image_ = daxa.device.create_image(
                {
                    .format = daxa::Format::R8G8B8A8_UNORM,
                    .size   = { size, size, 1 },
                    .usage  = daxa::ImageUsageFlagBits::SHADER_STORAGE |
                        daxa::ImageUsageFlagBits::TRANSFER_SRC,
                    .name = "mandelbulb_render_image",
                });

            // Create TaskImage wrapper
            task_render_image_ = daxa::TaskImage(
                {
                    .initial_images = { .images = std::array{ render_image_ } },
                    .name           = "task_mandelbulb_image",
                });

            last_extent_ = swapchain_extent;
        }

        graph.use_persistent_image(task_render_image_);

        // raymarch task
        graph.add_task(
            daxa::InlineTask::Compute("mandelbulb_raymarch")
                .writes(task_render_image_)
                .executes(
                    [this](daxa::TaskInterface ti)
                    {
                        auto& daxa       = render_ctx_->daxa_resources();
                        auto  image_info = daxa.device.info(render_image_).value();

                        // update camera stuff
                        if (!window_ || !camera_)
                            return;

                        const f32 move_speed       = 1.5f * engine_.delta_time();
                        const f32 look_sensitivity = 0.02f;

                        auto& pos = camera_->get<Pos3d>().val;

                        if (window_->is_key_down(Key::W))
                            pos += camera_->forward() * move_speed;
                        if (window_->is_key_down(Key::S))
                            pos -= camera_->forward() * move_speed;
                        if (window_->is_key_down(Key::A))
                            pos -= camera_->right() * move_speed;
                        if (window_->is_key_down(Key::D))
                            pos += camera_->right() * move_speed;
                        if (window_->is_key_down(Key::Q))
                            pos += camera_->up() * move_speed;
                        if (window_->is_key_down(Key::E))
                            pos -= camera_->up() * move_speed;

                        glm::vec2 delta = glm::vec2(window_->get_mouse_delta());

                        camera_->add_yaw(-delta.x * look_sensitivity);
                        camera_->add_pitch(delta.y * look_sensitivity);

                        ti.recorder.set_pipeline(*compute_pipeline_);

                        glm::mat4 inv_view_proj = glm::inverse(camera_->matrix());
                        glm::vec3 camera_pos    = camera_->get<Pos3d>().val;

                        // Push constants with camera and image info
                        struct MandelbulbPush {
                            daxa::ImageViewId image_id;
                            glm::u32vec2      frame_dim;
                            glm::mat4         inv_view_proj;
                            glm::vec3         camera_pos;
                            f32               time;
                        } push = {
                            .image_id      = render_image_.default_view(),
                            .frame_dim     = { image_info.size.x, image_info.size.y },
                            .inv_view_proj = inv_view_proj,
                            .camera_pos    = camera_pos,
                            .time          = (f32)sw_.elapsed(),
                        };

                        ti.recorder.push_constant(push);

                        u32 dispatch_x = (image_info.size.x + 7) / 8;
                        u32 dispatch_y = (image_info.size.y + 7) / 8;
                        ti.recorder.dispatch({ dispatch_x, dispatch_y, 1 });
                    }));

        // blit task
        graph.add_task(
            daxa::InlineTask::Transfer("mandelbulb_blit_to_swapchain")
                .reads(task_render_image_)
                .reads_writes(swapchain_image)
                .executes(
                    [this](daxa::TaskInterface ti)
                    {
                        auto& daxa       = render_ctx_->daxa_resources();
                        auto  image_info = daxa.device.info(render_image_).value();

                        ti.recorder.blit_image_to_image(
                            {
                                .src_image = ti.get(task_render_image_).ids[0],
                                .dst_image =
                                    ti.get(render_ctx_->swapchain_image()).ids[0],
                                .src_offsets = { { { 0, 0, 0 },
                                                   { static_cast<i32>(image_info.size.x),
                                                     static_cast<i32>(image_info.size.y),
                                                     1 } } },
                                .dst_offsets = { { { 0, 0, 0 },
                                                   { static_cast<i32>(image_info.size.x),
                                                     static_cast<i32>(image_info.size.y),
                                                     1 } } },
                            });
                    }));
    }
} // namespace v
