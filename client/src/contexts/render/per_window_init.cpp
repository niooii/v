//
// Created by niooi on 8/6/25.
//

#include <algorithm>
#include <contexts/render.h>
#include "init_vk.h"
#include <engine/engine.h>
#include <stdexcept>
#include <vulkan/vk_enum_string_helper.h>

constexpr u32 FRAMES_IN_FLIGHT = 2;

namespace v {
    WindowRenderResources::WindowRenderResources(
        Window* window, const VulkanResources* vulkan_resources) :
        vulkan_resources_(vulkan_resources)
    {
        LOG_INFO("Initializing per-window Vulkan resources...");

        // Create surface
        if (!window->create_vk_surface(
                vulkan_resources_->instance, vulkan_resources_->allocator, &surface))
        {
            throw std::runtime_error("Failed to create Vulkan surface");
        }

        // Create swapchain
        VkSwapchainCreateInfoKHR swapchain_create_info{};
        swapchain_create_info.sType         = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchain_create_info.surface       = surface;
        swapchain_create_info.minImageCount = FRAMES_IN_FLIGHT;
        swapchain_create_info.imageFormat   = VK_FORMAT_B8G8R8A8_SRGB; // Common format
        swapchain_create_info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

        auto window_size                       = window->size();
        swapchain_create_info.imageExtent      = { static_cast<u32>(window_size.x),
                                                   static_cast<u32>(window_size.y) };
        swapchain_create_info.imageArrayLayers = 1;
        swapchain_create_info.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchain_create_info.preTransform     = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        swapchain_create_info.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchain_create_info.presentMode      = VK_PRESENT_MODE_FIFO_KHR; // V-sync
        swapchain_create_info.clipped          = VK_TRUE;
        swapchain_create_info.oldSwapchain     = VK_NULL_HANDLE;

        swapchain_format = swapchain_create_info.imageFormat;
        swapchain_extent = swapchain_create_info.imageExtent;

        auto sc_res = vkCreateSwapchainKHR(
            vulkan_resources_->device, &swapchain_create_info,
            vulkan_resources_->allocator, &swapchain);
        if (sc_res != VK_SUCCESS)
        {
            LOG_ERROR("Failed to create swapchain: {}", string_VkResult(sc_res));
            throw std::runtime_error("Failed to create swapchain");
        }

        // Get swapchain images
        u32 actual_image_count;
        vkGetSwapchainImagesKHR(
            vulkan_resources_->device, swapchain, &actual_image_count, nullptr);
        swapchain_images.resize(actual_image_count);
        vkGetSwapchainImagesKHR(
            vulkan_resources_->device, swapchain, &actual_image_count, swapchain_images.data());

        // Create image views
        swapchain_image_views.resize(swapchain_images.size());
        for (size_t i = 0; i < swapchain_images.size(); i++)
        {
            VkImageViewCreateInfo view_info{};
            view_info.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            view_info.image    = swapchain_images[i];
            view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            view_info.format   = swapchain_format;
            view_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            view_info.subresourceRange.baseMipLevel   = 0;
            view_info.subresourceRange.levelCount     = 1;
            view_info.subresourceRange.baseArrayLayer = 0;
            view_info.subresourceRange.layerCount     = 1;

            if (vkCreateImageView(
                    vulkan_resources_->device, &view_info, vulkan_resources_->allocator,
                    &swapchain_image_views[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create image views");
            }
        }

        // Create command pool
        VkCommandPoolCreateInfo pool_info{};
        pool_info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        pool_info.queueFamilyIndex = vulkan_resources_->graphics_queue_family;

        if (vkCreateCommandPool(
                vulkan_resources_->device, &pool_info, vulkan_resources_->allocator,
                &command_pool) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create command pool");
        }

        // Create command buffers
        command_buffers.resize(FRAMES_IN_FLIGHT);
        VkCommandBufferAllocateInfo alloc_info{};
        alloc_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.commandPool        = command_pool;
        alloc_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandBufferCount = FRAMES_IN_FLIGHT;

        if (vkAllocateCommandBuffers(
                vulkan_resources_->device, &alloc_info, command_buffers.data()) !=
            VK_SUCCESS)
        {
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

        for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++)
        {
            if (vkCreateSemaphore(
                    vulkan_resources_->device, &semaphore_info,
                    vulkan_resources_->allocator,
                    &image_available_semaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(
                    vulkan_resources_->device, &semaphore_info,
                    vulkan_resources_->allocator,
                    &render_finished_semaphores[i]) != VK_SUCCESS ||
                vkCreateFence(
                    vulkan_resources_->device, &fence_info, vulkan_resources_->allocator,
                    &in_flight_fences[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create synchronization objects");
            }
        }

        LOG_INFO("Finish initializing vulkan stuff");
    }

    WindowRenderResources::~WindowRenderResources()
    {
        LOG_INFO("Cleaning up per-window Vulkan stuff..");

        // Clean up synchronization objects
        for (size_t i = 0; i < in_flight_fences.size(); i++)
        {
            if (in_flight_fences[i] != VK_NULL_HANDLE)
            {
                vkDestroyFence(
                    vulkan_resources_->device, in_flight_fences[i],
                    vulkan_resources_->allocator);
            }
            if (render_finished_semaphores[i] != VK_NULL_HANDLE)
            {
                vkDestroySemaphore(
                    vulkan_resources_->device, render_finished_semaphores[i],
                    vulkan_resources_->allocator);
            }
            if (image_available_semaphores[i] != VK_NULL_HANDLE)
            {
                vkDestroySemaphore(
                    vulkan_resources_->device, image_available_semaphores[i],
                    vulkan_resources_->allocator);
            }
        }

        // Command buffers are automatically freed when command pool is destroyed
        if (command_pool != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(
                vulkan_resources_->device, command_pool, vulkan_resources_->allocator);
        }

        // Clean up image views
        for (VkImageView image_view : swapchain_image_views)
        {
            if (image_view != VK_NULL_HANDLE)
            {
                vkDestroyImageView(
                    vulkan_resources_->device, image_view, vulkan_resources_->allocator);
            }
        }

        // Swapchain images are owned by swapchain and cleaned up automatically
        if (swapchain != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(
                vulkan_resources_->device, swapchain, vulkan_resources_->allocator);
        }

        if (surface != VK_NULL_HANDLE)
        {
            vkDestroySurfaceKHR(
                vulkan_resources_->instance, surface, vulkan_resources_->allocator);
        }

        LOG_INFO("Finished destroying per-window Vulkan resources");
    }
} // namespace v
