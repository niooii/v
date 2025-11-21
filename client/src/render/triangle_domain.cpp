//
// Created by niooi on 10/10/2025.
//

#include <engine/contexts/render/ctx.h>
#include <engine/contexts/render/init_vk.h>
#include <engine/engine.h>
#include <render/triangle_domain.h>

namespace v {
    void TriangleRenderer::init()
    {
        RenderDomain::init();

        auto& daxa = render_ctx_->daxa_resources();

        pipeline_ = daxa.pipeline_manager.add_raster_pipeline2({
            .vertex_shader_info   = daxa::ShaderCompileInfo2{
                .source = daxa::ShaderFile{ "triangle.glsl" },
                .defines = { { "VERTEX_SHADER", "1" } },
            },
            .fragment_shader_info = daxa::ShaderCompileInfo2{
                .source = daxa::ShaderFile{ "triangle.glsl" },
                .defines = { { "FRAGMENT_SHADER", "1" } },
            },
            .color_attachments = {
                { .format = render_ctx_->get_swapchain_format() },
            },
            .raster = {
                .face_culling = daxa::FaceCullFlagBits::NONE,
            },
            .name = "triangle_pipeline",
        }).value();

        LOG_INFO("TriangleRenderer initialized");
    }

    void TriangleRenderer::add_render_tasks(daxa::TaskGraph& graph)
    {
        auto& swapchain_image = render_ctx_->swapchain_image();

        graph.add_task(
            daxa::Task::Raster("rainbow_triangle")
                // Specify view type so TaskGraph generates a valid image_view for runtime
                // swapchain images
                .color_attachment
                .reads_writes(daxa::ImageViewType::REGULAR_2D, swapchain_image)
                .executes(
                    [this](daxa::TaskInterface ti)
                    {
                        auto&      swapchain_image = render_ctx_->swapchain_image();
                        auto const image_id        = ti.get(swapchain_image).ids[0];
                        auto const image_view      = ti.get(swapchain_image).view_ids[0];
                        auto const image_info = ti.device.image_info(image_id).value();

                        auto render_recorder = std::move(ti.recorder).begin_renderpass({
                        .color_attachments = {
                            {
                                .image_view = image_view,
                                .load_op = daxa::AttachmentLoadOp::LOAD,
                            },
                        },
                        .render_area = {
                            .x = 0,
                            .y = 0,
                            .width = image_info.size.x,
                            .height = image_info.size.y
                        },
                    });

                        render_recorder.set_pipeline(*pipeline_);
                        render_recorder.draw({ .vertex_count = 3 });

                        ti.recorder = std::move(render_recorder).end_renderpass();
                    }));
    }
} // namespace v
