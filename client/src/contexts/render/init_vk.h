//
// Created by niooi on 8/7/25.
//

#pragma once

#include <defs.h>
#include <vulkan/vulkan.h>

namespace v {
    class Window;
    class WindowContext;
    class Engine;

    /// The vulkan resources used globally
    struct VulkanResources {
        VkInstance             instance;
        VkAllocationCallbacks* allocator;
        VkPhysicalDevice       physical_device;
        VkDevice               device;

        VkQueue graphics_queue;
        VkQueue present_queue;
        VkQueue compute_queue;
        VkQueue transfer_queue;

        uint32_t graphics_queue_family;
        uint32_t present_queue_family;
        uint32_t compute_queue_family;
        uint32_t transfer_queue_family;

        explicit VulkanResources(Engine& engine);
        ~VulkanResources();

        VulkanResources(const VulkanResources&) = delete;
        VulkanResources& operator=(const VulkanResources&) = delete;
    };
}