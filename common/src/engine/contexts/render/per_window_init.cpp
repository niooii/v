//
// Created by niooi on 8/6/25.
//

#include <algorithm>
#include "daxa/utils/task_graph.hpp"
#include "daxa/utils/task_graph_types.hpp"
#include "defs.h"
// this isnt needed but im tryna fix some weird intellisense bug
#define DAXA_ENABLE_UTILS_TASK_GRAPH 1
#include <engine/contexts/render/ctx.h>
#include <engine/engine.h>
#include <stdexcept>
#include "engine/contexts/render/init_vk.h"


namespace v {
    WindowRenderResources::WindowRenderResources(
        Window* window, DaxaResources* daxa_resources) : daxa_resources_(daxa_resources)
    {
        LOG_INFO("Initializing per-window Daxa stuff...");
        // Get native window handle from SDL3 window properties
        daxa::NativeWindowHandle   native_handle;
        daxa::NativeWindowPlatform native_platform;

        SDL_PropertiesID window_props = SDL_GetWindowProperties(window->get_sdl_window());

#if defined(_WIN32)
        void* hwnd = SDL_GetPointerProperty(
            window_props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
        if (!hwnd)
        {
            throw std::runtime_error("Failed to get Win32 HWND from SDL window");
        }
        native_handle   = reinterpret_cast<daxa::NativeWindowHandle>(hwnd);
        native_platform = daxa::NativeWindowPlatform::WIN32_API;
#elif defined(__linux__)
        // wayland or x11
        const char* video_driver = SDL_GetCurrentVideoDriver();
        if (!video_driver)
        {
            throw std::runtime_error("Failed to get current SDL video driver");
        }

        if (strcmp(video_driver, "wayland") == 0)
        {
            void* wl_surface = SDL_GetPointerProperty(
                window_props, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr);
            if (!wl_surface)
                throw std::runtime_error("Failed to get Wayland surface from SDL window");
            native_handle   = reinterpret_cast<daxa::NativeWindowHandle>(wl_surface);
            native_platform = daxa::NativeWindowPlatform::WAYLAND_API;
            LOG_INFO("Using Wayland video driver");
        }
        else if (strcmp(video_driver, "x11") == 0)
        {
            Uint64 x11_window =
                SDL_GetNumberProperty(window_props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
            if (x11_window == 0)
                throw std::runtime_error("Failed to get X11 Window ID from SDL window");
            native_handle   = reinterpret_cast<daxa::NativeWindowHandle>(x11_window);
            native_platform = daxa::NativeWindowPlatform::XLIB_API;
            LOG_INFO("Using X11 video driver");
        }
        else
        {
            throw std::runtime_error(
                fmt::format("Unsupported SDL video driver '{}'", video_driver));
        }
#else
        throw std::runtime_error("Unsupported platform for now");
#endif
        // fallback present modes
        daxa::PresentMode present_modes[] = {
            daxa::PresentMode::MAILBOX,
            daxa::PresentMode::FIFO,
            daxa::PresentMode::IMMEDIATE,
        };

        bool swapchain_created = false;
        for (auto present_mode : present_modes)
        {
            try
            {
                swapchain = daxa_resources_->device.create_swapchain(
                    {
                        .native_window           = native_handle,
                        .native_window_platform  = native_platform,
                        .surface_format_selector = [](daxa::Format format) -> i32
                        {
                            switch (format)
                            {
                            case daxa::Format::B8G8R8A8_SRGB:
                                return 100;
                            case daxa::Format::R8G8B8A8_SRGB:
                                return 90;
                            case daxa::Format::B8G8R8A8_UNORM:
                                return 80;
                            case daxa::Format::R8G8B8A8_UNORM:
                                return 70;
                            default:
                                return daxa::default_format_score(format);
                            }
                        },
                        .present_mode = present_mode,
                        .image_usage  = daxa::ImageUsageFlagBits::COLOR_ATTACHMENT,
                        .max_allowed_frames_in_flight = FRAMES_IN_FLIGHT,
                        .name                         = "swapchain",
                    });
                swapchain_created = true;
                LOG_INFO(
                    "Created swapchain with present mode: {}",
                    static_cast<int>(present_mode));
                break;
            }
            catch (const std::exception& e)
            {
                LOG_DEBUG(
                    "Failed to create swapchain with present mode {}: {}\nRolling up one "
                    "present mode",
                    static_cast<int>(present_mode), e.what());
                continue;
            }
        }

        if (!swapchain_created)
        {
            LOG_ERROR("Failed to create swapchain with ANY present mode?");
            throw std::runtime_error("Failed to create swapchain");
        }

        task_swapchain_image = daxa::TaskImage{ {
            .swapchain_image = true,
            .name            = "swapchain img",
        } };

        // resize swapchain on window resize

        window->on_resize.connect<&WindowRenderResources::resize>(this);


        LOG_TRACE("created swapchain, creating task graph now");

        render_graph = daxa::TaskGraph(
            {
                .device    = daxa_resources_->device,
                .swapchain = swapchain,
                .name      = "main loop graph",
            });

        render_graph.use_persistent_image(task_swapchain_image);

        // test clear task
        render_graph.add_task({
            .attachments = {
                daxa::inl_attachment(daxa::TaskAccessConsts::COLOR_ATTACHMENT, daxa::ImageViewType::REGULAR_2D, task_swapchain_image),
            },
            .task = [=](daxa::TaskInterface ti)
            {
                auto& render_target = task_swapchain_image;
                auto const size = ti.device.info(ti.get(render_target).ids[0]).value().size;

                daxa::RenderCommandRecorder render_recorder = std::move(ti.recorder).begin_renderpass({
                    .color_attachments = std::array{
                        daxa::RenderAttachmentInfo{
                            .image_view = ti.get(render_target).view_ids[0],
                            .load_op = daxa::AttachmentLoadOp::CLEAR,
                            .clear_value = std::array<daxa::f32, 4>{0.1f, 0.0f, 0.5f, 1.0f},
                        },
                    },
                    .render_area = {.width = size.x, .height = size.y},
                });

                ti.recorder = std::move(render_recorder).end_renderpass();
            },
            .name = "draw vertices",
        });

        render_graph.submit({});
        render_graph.present({});
        render_graph.complete({});

        LOG_INFO("Finished initializing per-window render stuff");
    }

    WindowRenderResources::~WindowRenderResources()
    {
        LOG_INFO("Cleaning up swapchain and stuff...");

        if (daxa_resources_->device.is_valid())
        {
            daxa_resources_->device.wait_idle();
        }
    }

    void WindowRenderResources::render()
    {
        auto swapchain_img = swapchain.acquire_next_image();
        if (swapchain_img.is_empty())
        {
            LOG_WARN("No image acquired from swapchain..??");
            return;
        }

        task_swapchain_image.set_images({ .images = std::span{ &swapchain_img, 1 } });

        render_graph.execute({});
    }

    void WindowRenderResources::resize()
    {
        LOG_TRACE("resized swapchain");
        swapchain.resize();
    }
} // namespace v
