//
// Created by niooi on 8/6/25.
//

#include <contexts/render.h>
#include <engine/engine.h>
#include <stdexcept>
#include <algorithm>

constexpr u32 FRAMES_IN_FLIGHT = 2;

namespace v {
    WindowRenderResources::WindowRenderResources(Window* window, Engine& engine) 
        : current_frame(0) 
    {
        LOG_INFO("Initializing per-window Vulkan resources...");

        RenderContext* render_context = engine.get_context<RenderContext>();
        if (!render_context) {
            throw std::runtime_error("Failed to get RenderContext");
        }

        // Create surface
        if (!window->create_vk_surface(render_context->instance_, render_context->allocator_, &surface)) {
            throw std::runtime_error("Failed to create Vulkan surface");
        }

        // Check surface support
        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(
            render_context->physical_device_, 
            render_context->graphics_queue_family_, surface, &present_support);

        if (!present_support) {
            LOG_WARN("Present queue may not support this surface");
        }

        // Get surface capabilities
        VkSurfaceCapabilitiesKHR capabilities;
        // We need to get physical device handle - this is missing from RenderContext
        // For now, let's create a minimal swapchain setup

        // Create swapchain
        VkSwapchainCreateInfoKHR swapchain_create_info{};
        swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchain_create_info.surface = surface;
        swapchain_create_info.minImageCount = FRAMES_IN_FLIGHT;
        swapchain_create_info.imageFormat = VK_FORMAT_B8G8R8A8_SRGB; // Common format
        swapchain_create_info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        
        auto window_size = window->size();
        swapchain_create_info.imageExtent = { static_cast<u32>(window_size.x), static_cast<u32>(window_size.y) };
        swapchain_create_info.imageArrayLayers = 1;
        swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchain_create_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchain_create_info.presentMode = VK_PRESENT_MODE_FIFO_KHR; // V-sync
        swapchain_create_info.clipped = VK_TRUE;
        swapchain_create_info.oldSwapchain = VK_NULL_HANDLE;

        swapchain_format = swapchain_create_info.imageFormat;
        swapchain_extent = swapchain_create_info.imageExtent;

        if (vkCreateSwapchainKHR(render_context->device_, &swapchain_create_info, render_context->allocator_, &swapchain) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create swapchain");
        }

        // Get swapchain images
        u32 image_count;
        vkGetSwapchainImagesKHR(render_context->device_, swapchain, &image_count, nullptr);
        swapchain_images.resize(image_count);
        vkGetSwapchainImagesKHR(render_context->device_, swapchain, &image_count, swapchain_images.data());

        // Create image views
        swapchain_image_views.resize(swapchain_images.size());
        for (size_t i = 0; i < swapchain_images.size(); i++) {
            VkImageViewCreateInfo view_info{};
            view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            view_info.image = swapchain_images[i];
            view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            view_info.format = swapchain_format;
            view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            view_info.subresourceRange.baseMipLevel = 0;
            view_info.subresourceRange.levelCount = 1;
            view_info.subresourceRange.baseArrayLayer = 0;
            view_info.subresourceRange.layerCount = 1;

            if (vkCreateImageView(render_context->device_, &view_info, render_context->allocator_, &swapchain_image_views[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create image views");
            }
        }

        // Create command pool
        VkCommandPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        pool_info.queueFamilyIndex = render_context->graphics_queue_family_;

        if (vkCreateCommandPool(render_context->device_, &pool_info, render_context->allocator_, &command_pool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create command pool");
        }

        // Create command buffers
        command_buffers.resize(FRAMES_IN_FLIGHT);
        VkCommandBufferAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.commandPool = command_pool;
        alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandBufferCount = FRAMES_IN_FLIGHT;

        if (vkAllocateCommandBuffers(render_context->device_, &alloc_info, command_buffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate command buffers");
        }

        // Create synchronization objects
        image_available_semaphores.resize(FRAMES_IN_FLIGHT);
        render_finished_semaphores.resize(FRAMES_IN_FLIGHT);
        in_flight_fences.resize(FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphore_info{};
        semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fence_info{};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore(render_context->device_, &semaphore_info, render_context->allocator_, &image_available_semaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(render_context->device_, &semaphore_info, render_context->allocator_, &render_finished_semaphores[i]) != VK_SUCCESS ||
                vkCreateFence(render_context->device_, &fence_info, render_context->allocator_, &in_flight_fences[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create synchronization objects");
            }
        }

        LOG_INFO("Per-window Vulkan resources initialized successfully");
    }

    WindowRenderResources::~WindowRenderResources() {
        // TODO: Implement cleanup
        LOG_INFO("Destroying per-window Vulkan resources...");
    }
}