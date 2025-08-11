//
// Created by niooi on 8/7/25.
//

#include "init_vk.h"
#include <contexts/window.h>
#include <engine/engine.h>
#include <stdexcept>


namespace v {


    DaxaResources::DaxaResources(Engine& engine)
    {
        LOG_INFO("Initializing Daxa context...");

        // Verify window context exists (needed for eventual swapchain creation)
        auto window_ctx = engine.read_context<WindowContext>();
        if (!window_ctx)
        {
            throw std::runtime_error(
                "WindowContext does not exist - DaxaResources depends on a window "
                "being created first");
        }

        Window* temp_window = (*window_ctx)->get_window();
        if (!temp_window)
        {
            throw std::runtime_error(
                "DaxaResources depends on a window being created first");
        }

        // Create daxa instance
        instance = daxa::create_instance({
            .engine_name = "vengine",
            .app_name = "vengine"
        });

        LOG_DEBUG("Daxa instance created successfully");

        daxa::DeviceInfo2 device_info = {
            .max_allowed_buffers = 16384,
            .max_allowed_images = 16384,
            .name = "vengine_device",
        };

        // Let daxa choose a suitable device
        try 
        {
            device = instance.create_device_2(instance.choose_device({}, device_info));
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Failed to create Daxa device: {}", e.what());
            throw std::runtime_error("Failed to create Daxa device");
        }

        LOG_INFO("Daxa context initialized successfully");
    }

    DaxaResources::~DaxaResources()
    {
        LOG_INFO("Cleaning up DaxaResources...");

        // Daxa handles cleanup automatically through RAII
        if (device.is_valid())
        {
            device.wait_idle();
            device.collect_garbage();
        }

        LOG_INFO("DaxaResources cleanup complete");
    }
} // namespace v
