//
// Created by niooi on 10/10/2025.
//

#include <engine/contexts/render/ctx.h>
#include <engine/contexts/render/default_render_domain.h>
#include <engine/engine.h>

namespace v {
    DefaultRenderDomain::DefaultRenderDomain(Engine& engine) :
        RenderDomain(engine, "DefaultRender")
    {
        LOG_INFO("DefaultRenderDomain initialized");
    }

    void DefaultRenderDomain::add_render_tasks(daxa::TaskGraph& graph)
    {
        auto& swapchain_image = render_ctx_->swapchain_image();

        graph.add_task(
            daxa::Task::Raster("default_clear")
                .color_attachment.writes(daxa::ImageViewType::REGULAR_2D, swapchain_image)
                .raster_shader.writes(render_ctx_->order_token())
                .executes([swapchain_image](daxa::TaskInterface ti)
                {
                    auto const size =
                        ti.device.info(ti.get(swapchain_image).ids[0]).value().size;

                    daxa::RenderCommandRecorder render_recorder =
                        std::move(ti.recorder).begin_renderpass({
                            .color_attachments = std::array{
                                daxa::RenderAttachmentInfo{
                                    .image_view  = ti.get(swapchain_image).view_ids[0],
                                    .load_op     = daxa::AttachmentLoadOp::CLEAR,
                                    .clear_value = std::array<daxa::f32, 4>{ 0.1f, 0.1f, 0.1f, 1.0f },
                                },
                            },
                            .render_area = { .width = size.x, .height = size.y },
                        });

                    ti.recorder = std::move(render_recorder).end_renderpass();
                }));
    }
} // namespace v
