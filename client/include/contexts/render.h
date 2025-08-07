//
// Created by niooi on 7/30/2025.
//

#pragma once

#include <defs.h>
#include <domain/context.h>
#include <domain/domain.h>
#include <contexts/window.h>
#include <vulkan/vulkan.h>
#include <vector>

namespace v {
    typedef void(RenderComponentFnRender)(
        Engine*, class RenderContext*, Window*);

    typedef void(RenderComponentFnResize)(
        Engine*, RenderContext*, Window*);

    struct RenderComponent {
        std::function<RenderComponentFnRender> pre_render{};
        std::function<RenderComponentFnRender> render{};
        std::function<RenderComponentFnRender> display{};
        std::function<RenderComponentFnRender> resize{};
    };

    /// The vulkan resources used globally

    /// The vulkan resources used per-window
    struct WindowRenderResources {
        WindowRenderResources(Window* window, Engine& engine);
        ~WindowRenderResources();

        VkSurfaceKHR surface;
        VkSwapchainKHR swapchain;
        
        std::vector<VkImage> swapchain_images;
        std::vector<VkImageView> swapchain_image_views;
        VkFormat swapchain_format;
        VkExtent2D swapchain_extent;
        
        std::vector<VkCommandBuffer> command_buffers;
        VkCommandPool command_pool;
        
        std::vector<VkSemaphore> image_available_semaphores;
        std::vector<VkSemaphore> render_finished_semaphores;
        std::vector<VkFence> in_flight_fences;
        
        u32 current_frame;
    };

    class RenderContext : public Context {
        friend struct WindowRenderResources;
    public:
        explicit RenderContext(Engine& engine);

        /// Creates and attaches a RenderComponent to an entity, usually a Domain.
        /// @return A reference to the newly created component for modification.
        /// The reference must not be stored, and is safe to use as long as only
        /// one thread is creating a RenderComponent at a time.
        RenderComponent& create_component(entt::entity id);

    private:
        VkInstance instance_;
        VkAllocationCallbacks* allocator_;
        VkPhysicalDevice physical_device_;
        VkDevice device_;
        
        VkQueue graphics_queue_;
        VkQueue present_queue_;
        VkQueue compute_queue_;
        VkQueue transfer_queue_;
        
        uint32_t graphics_queue_family_;
        uint32_t present_queue_family_;
        uint32_t compute_queue_family_;
        uint32_t transfer_queue_family_;
    };
} // namespace v
