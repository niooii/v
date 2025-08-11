//
// Created by niooi on 8/6/25.
//

#include <algorithm>
#include <contexts/render.h>
#include <engine/engine.h>
#include <stdexcept>
#include "init_vk.h"


namespace v {
    WindowRenderResources::WindowRenderResources(
        Window* window, DaxaResources* daxa_resources) : daxa_resources_(daxa_resources)
    {
        LOG_INFO("Initializing per-window Daxa resources...");

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
                fmt::format(
                    "Unsupported SDL video driver '{}'",
                    video_driver));
        }
#else
        throw std::runtime_error("Unsupported platform for now");
#endif

        // Create swapchain
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
                    .present_mode = daxa::PresentMode::MAILBOX, 
                    .image_usage  = daxa::ImageUsageFlagBits::COLOR_ATTACHMENT,
                    .max_allowed_frames_in_flight = FRAMES_IN_FLIGHT,
                    .name                         = "swapchain",
                });
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Failed to create Daxa swapchain: {}", e.what());
            throw std::runtime_error("Failed to create Daxa swapchain");
        }

        LOG_INFO("Finished initializing Daxa per-window resources");
    }

    WindowRenderResources::~WindowRenderResources()
    {
        LOG_INFO("Cleaning up per-window Daxa resources...");

        if (daxa_resources_->device.is_valid())
        {
            daxa_resources_->device.wait_idle();
        }
    }
} // namespace v
