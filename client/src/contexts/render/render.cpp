//
// Created by niooi on 7/30/2025.
//

#include <VkBootstrap.h>
#include <contexts/render.h>
#include <stdexcept>

#include "engine/engine.h"

static u32 REQUIRED_VK_MAJOR = 1;
static u32 REQUIRED_VK_MINOR = 3;
static u32 REQUIRED_VK_PATCH = 0;

namespace v {
    RenderContext::RenderContext(Engine& engine) : Context(engine)
    {
        LOG_INFO("Initializing Vulkan context...");

        vkb::InstanceBuilder instance_builder;

        auto instance_ret =
            instance_builder.set_app_name("vengine")
                .set_engine_name("vengine")
                .require_api_version(REQUIRED_VK_MAJOR, REQUIRED_VK_MINOR, REQUIRED_VK_PATCH)
                .request_validation_layers(true)
                .use_default_debug_messenger()
                .build();

        if (!instance_ret)
        {
            LOG_ERROR(
                "Failed to create Vulkan instance: {}", instance_ret.error().message());
            throw std::runtime_error("Failed to create Vulkan instance");
        }

        vkb::Instance vkb_instance = instance_ret.value();
        instance_                  = vkb_instance.instance;
        /// TODO!
        allocator_ = nullptr;
        LOG_DEBUG("Vulkan instance created successfully");

        VkPhysicalDeviceDynamicRenderingFeatures dynamic_rendering_features{};
        dynamic_rendering_features.sType =
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
        dynamic_rendering_features.dynamicRendering = VK_TRUE;

        u32 device_count = 0;
        vkEnumeratePhysicalDevices(instance_, &device_count, nullptr);

        if (device_count == 0)
        {
            LOG_ERROR("Failed to find GPUs with Vulkan support");
            throw std::runtime_error("Failed to find GPUs with Vulkan support");
        }

        std::vector<VkPhysicalDevice> devices(device_count);
        vkEnumeratePhysicalDevices(instance_, &device_count, devices.data());

        VkPhysicalDevice selected_device = VK_NULL_HANDLE;

        // Select the first discrete GPU if available, otherwise any GPU
        for (const auto& device : devices)
        {
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(device, &properties);

            if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            {
                selected_device = device;
                break;
            }
        }

        if (selected_device == VK_NULL_HANDLE)
        {
            selected_device = devices[0];
        }

        physical_device_ = selected_device;

        // Find queue families
        uint32_t queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(
            selected_device, &queue_family_count, nullptr);
        std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(
            selected_device, &queue_family_count, queue_families.data());

        // Find graphics queue family
        graphics_queue_family_ = UINT32_MAX;
        for (uint32_t i = 0; i < queue_family_count; i++)
        {
            if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                graphics_queue_family_ = i;
                break;
            }
        }

        if (graphics_queue_family_ == UINT32_MAX)
        {
            throw std::runtime_error("Failed to find graphics queue family");
        }

        // Use graphics queue for all others by default
        present_queue_family_  = graphics_queue_family_;
        compute_queue_family_  = graphics_queue_family_;
        transfer_queue_family_ = graphics_queue_family_;

        // Create device with queue
        float                   queue_priority = 1.0f;
        VkDeviceQueueCreateInfo queue_create_info{};
        queue_create_info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = graphics_queue_family_;
        queue_create_info.queueCount       = 1;
        queue_create_info.pQueuePriorities = &queue_priority;

        VkDeviceCreateInfo device_create_info{};
        device_create_info.sType                 = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        device_create_info.pNext                 = &dynamic_rendering_features;
        device_create_info.queueCreateInfoCount  = 1;
        device_create_info.pQueueCreateInfos     = &queue_create_info;
        device_create_info.enabledLayerCount     = 0;
        device_create_info.enabledExtensionCount = 0;

        VkResult result =
            vkCreateDevice(selected_device, &device_create_info, allocator_, &device_);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create logical device");
        }

        // Get queue handles
        vkGetDeviceQueue(device_, graphics_queue_family_, 0, &graphics_queue_);
        present_queue_  = graphics_queue_;
        compute_queue_  = graphics_queue_;
        transfer_queue_ = graphics_queue_;

        LOG_INFO(
            "Vulkan context initialized successfully with queues: Graphics({}) "
            "Present({}) Compute({}) Transfer({})",
            graphics_queue_family_, present_queue_family_, compute_queue_family_,
            transfer_queue_family_);

        // Init rendering stuff for the window
        if (auto window_ctx = engine_.write_context<WindowContext>())
        {
            Window* win = (*window_ctx)->get_window();
            if (!win)
            {
                LOG_WARN("Found no existing window, creating one");
                win = (*window_ctx)->create_window("Window", { 600, 600 }, { 200, 200 });
            }

            // TODO! Create window specific render resources
        }
        else
        {
            throw std::runtime_error("Create WindowContext before creating RenderContext");
        }
    }
} // namespace v
