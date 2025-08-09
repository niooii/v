//
// Created by niooi on 8/7/25.
//

#include "init_vk.h"
#include <SDL3/SDL_vulkan.h>
#include <VkBootstrap.h>
#include <contexts/window.h>
#include <cstring>
#include <engine/engine.h>
#include <set>
#include <stdexcept>

static u32 REQUIRED_VK_MAJOR = 1;
static u32 REQUIRED_VK_MINOR = 3;
static u32 REQUIRED_VK_PATCH = 0;

namespace v {
    // Helper function to check if device supports required extensions
    bool check_device_extension_support(VkPhysicalDevice device)
    {
        uint32_t extension_count;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

        std::vector<VkExtensionProperties> available_extensions(extension_count);
        vkEnumerateDeviceExtensionProperties(
            device, nullptr, &extension_count, available_extensions.data());

        // Required device extensions
        std::vector<const char*> required_extensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            // Ray tracing extensions
            VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
            VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
            VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
            // Dynamic rendering
            VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME
        };

        LOG_INFO("  Checking device extensions ({} available):", extension_count);
        for (const char* required : required_extensions)
        {
            bool found = false;
            for (const auto& available : available_extensions)
            {
                if (strcmp(required, available.extensionName) == 0)
                {
                    found = true;
                    break;
                }
            }
            LOG_INFO("    {}: {}", required, found ? "supported" : "missing");
            if (!found)
            {
                return false;
            }
        }
        return true;
    }

    // Helper function to find queue families
    struct QueueFamilyIndices {
        uint32_t graphics_family = UINT32_MAX;
        uint32_t present_family  = UINT32_MAX;
        uint32_t compute_family  = UINT32_MAX;
        uint32_t transfer_family = UINT32_MAX;

        bool is_complete()
        {
            return graphics_family != UINT32_MAX && present_family != UINT32_MAX;
        }
    };

    QueueFamilyIndices
    find_queue_families_helper(VkPhysicalDevice device, VkSurfaceKHR surface)
    {
        QueueFamilyIndices indices;

        // Validate surface
        LOG_INFO("  Surface handle: {}", surface != VK_NULL_HANDLE ? "valid" : "invalid");

        uint32_t queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);
        std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(
            device, &queue_family_count, queue_families.data());

        LOG_INFO("  Checking {} queue families for surface support:", queue_family_count);

        for (uint32_t i = 0; i < queue_family_count; i++)
        {
            if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                indices.graphics_family = i;
                LOG_INFO("    Queue family {} has graphics", i);
            }

            VkBool32 present_support = false;
            VkResult result          = vkGetPhysicalDeviceSurfaceSupportKHR(
                device, i, surface, &present_support);
            LOG_INFO(
                "    Queue family {} present support: {} (result={})", i,
                present_support ? "yes" : "no", static_cast<int>(result));

            if (present_support)
            {
                indices.present_family = i;
            }

            // Find dedicated compute queue (prefer non-graphics)
            if ((queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT) &&
                !(queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
            {
                indices.compute_family = i;
            }

            // Find dedicated transfer queue (prefer non-graphics, non-compute)
            if ((queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT) &&
                !(queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
                !(queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT))
            {
                indices.transfer_family = i;
            }
        }

        // Fallback for compute queue
        if (indices.compute_family == UINT32_MAX)
        {
            for (uint32_t i = 0; i < queue_family_count; i++)
            {
                if (queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
                {
                    indices.compute_family = i;
                    break;
                }
            }
        }

        // Fallback for transfer queue
        if (indices.transfer_family == UINT32_MAX)
        {
            for (uint32_t i = 0; i < queue_family_count; i++)
            {
                if (queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
                {
                    indices.transfer_family = i;
                    break;
                }
            }
        }

        // Final fallback - use graphics queue if no present queue found
        if (indices.present_family == UINT32_MAX)
        {
            LOG_WARN(
                "  No queue family reports surface support - falling back to graphics "
                "queue");
            LOG_WARN("  This might be a driver/compositor issue, trying anyway");
            indices.present_family = indices.graphics_family;
        }

        LOG_INFO(
            "  Final indices: Graphics={}, Present={}, Compute={}, Transfer={}",
            indices.graphics_family, indices.present_family, indices.compute_family,
            indices.transfer_family);

        return indices;
    }

    bool is_device_suitable(VkPhysicalDevice device, VkSurfaceKHR surface)
    {
        QueueFamilyIndices indices = find_queue_families_helper(device, surface);

        LOG_INFO("    Queue families complete: {}", indices.is_complete() ? "yes" : "no");
        LOG_INFO(
            "      Graphics: {}, Present: {}", indices.graphics_family,
            indices.present_family);

        bool extensions_supported = check_device_extension_support(device);
        LOG_INFO("    Extensions supported: {}", extensions_supported ? "yes" : "no");

        bool swapchain_adequate = false;
        if (extensions_supported)
        {
            uint32_t format_count;
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);

            uint32_t present_mode_count;
            vkGetPhysicalDeviceSurfacePresentModesKHR(
                device, surface, &present_mode_count, nullptr);

            swapchain_adequate = format_count != 0 && present_mode_count != 0;
        }

        VkPhysicalDeviceFeatures supported_features;
        vkGetPhysicalDeviceFeatures(device, &supported_features);
        LOG_INFO(
            "    Anisotropy support: {}",
            supported_features.samplerAnisotropy ? "yes" : "no");

        bool suitable = indices.is_complete() && extensions_supported &&
            swapchain_adequate && supported_features.samplerAnisotropy;
        LOG_INFO("    Overall suitable: {}", suitable ? "yes" : "no");

        return suitable;
    }

    VulkanResources::VulkanResources(Engine& engine)
    {
        LOG_INFO("Initializing Vulkan context...");

        // Get required Vulkan extensions from SDL
        std::vector<const char*> required_extensions;
        auto                     window_ctx = engine.read_context<WindowContext>();
        if (!window_ctx)
        {
            throw std::runtime_error(
                "WindowContext does not exist - VulkanResources depends on a window "
                "being created first");
        }

        Window* temp_window = (*window_ctx)->get_window();
        if (!temp_window)
        {
            throw std::runtime_error(
                "VulkanResources depends on a window being created first");
        }

        // SDL3 function to get required Vulkan instance extensions
        uint32_t           extension_count = 0;
        const char* const* extensions =
            SDL_Vulkan_GetInstanceExtensions(&extension_count);

        if (!extensions || extension_count == 0)
        {
            LOG_ERROR("Failed to query required Vulkan extensions from SDL");
            throw std::runtime_error(
                "Failed to query required Vulkan extensions from SDL");
        }

        LOG_INFO("SDL requires {} Vulkan instance extensions:", extension_count);
        for (uint32_t i = 0; i < extension_count; i++)
        {
            required_extensions.push_back(extensions[i]);
            LOG_INFO("  - {}", extensions[i]);
        }

        vkb::InstanceBuilder instance_builder;

        // Add SDL-required extensions to instance builder
        for (const char* ext : required_extensions)
        {
            instance_builder.enable_extension(ext);
        }

        auto instance_ret =
            instance_builder.set_app_name("vengine")
                .set_engine_name("vengine")
                .require_api_version(
                    REQUIRED_VK_MAJOR, REQUIRED_VK_MINOR, REQUIRED_VK_PATCH)
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
        instance                   = vkb_instance.instance;
        allocator                  = nullptr;
        LOG_DEBUG("Vulkan instance created successfully");

        // Set up device features with ray tracing and dynamic rendering
        VkPhysicalDeviceFeatures device_features{};
        device_features.samplerAnisotropy = VK_TRUE;

        // Ray tracing pipeline features
        VkPhysicalDeviceRayTracingPipelinePropertiesKHR rt_pipeline_properties{};
        rt_pipeline_properties.sType =
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;

        VkPhysicalDeviceRayTracingPipelineFeaturesKHR rt_pipeline_features{};
        rt_pipeline_features.sType =
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
        rt_pipeline_features.rayTracingPipeline                  = VK_TRUE;
        rt_pipeline_features.rayTracingPipelineTraceRaysIndirect = VK_TRUE;
        rt_pipeline_features.rayTraversalPrimitiveCulling        = VK_TRUE;

        // Acceleration structure features
        VkPhysicalDeviceAccelerationStructureFeaturesKHR as_features{};
        as_features.sType =
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
        as_features.accelerationStructure                                 = VK_TRUE;
        as_features.accelerationStructureCaptureReplay                    = VK_FALSE;
        as_features.accelerationStructureIndirectBuild                    = VK_FALSE;
        as_features.accelerationStructureHostCommands                     = VK_FALSE;
        as_features.descriptorBindingAccelerationStructureUpdateAfterBind = VK_FALSE;

        // Dynamic rendering features
        VkPhysicalDeviceDynamicRenderingFeatures dynamic_rendering_features{};
        dynamic_rendering_features.sType =
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
        dynamic_rendering_features.dynamicRendering = VK_TRUE;

        // Vulkan 1.2 features (needed for some ray tracing functionality)
        VkPhysicalDeviceVulkan12Features vulkan12_features{};
        vulkan12_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        vulkan12_features.bufferDeviceAddress = VK_TRUE;

        // Chain the features together
        vulkan12_features.pNext    = &as_features;
        as_features.pNext          = &rt_pipeline_features;
        rt_pipeline_features.pNext = &dynamic_rendering_features;

        u32 device_count = 0;
        vkEnumeratePhysicalDevices(instance, &device_count, nullptr);

        if (device_count == 0)
        {
            LOG_ERROR("Failed to find GPUs with Vulkan support");
            throw std::runtime_error("Failed to find GPUs with Vulkan support");
        }

        std::vector<VkPhysicalDevice> devices(device_count);
        vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

        VkPhysicalDevice selected_device = VK_NULL_HANDLE;

        // Create temporary surface for device selection
        VkSurfaceKHR temp_surface = VK_NULL_HANDLE;
        if (!temp_window->create_vk_surface(instance, allocator, &temp_surface))
        {
            throw std::runtime_error(
                "Failed to create temporary surface for device selection");
        }

        selected_device = VK_NULL_HANDLE;
        LOG_INFO("Checking {} physical devices", device_count);

        for (const auto& device : devices)
        {
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(device, &properties);
            LOG_INFO(
                "Testing device: {} (type={})", properties.deviceName,
                static_cast<int>(properties.deviceType));

            if (!is_device_suitable(device, temp_surface))
            {
                LOG_INFO("Not using device: {}", properties.deviceName);
                continue;
            }

            selected_device = device;

            if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            {
                LOG_INFO("Using discrete GPU: {}", properties.deviceName);
                break;
            }
        }

        if (selected_device == VK_NULL_HANDLE)
        {
            throw std::runtime_error(
                "Failed to find a suitable physical device with surface support");
        }

        physical_device = selected_device;

        QueueFamilyIndices indices =
            find_queue_families_helper(selected_device, temp_surface);
        graphics_queue_family = indices.graphics_family;
        present_queue_family  = indices.present_family;
        compute_queue_family  = indices.compute_family;
        transfer_queue_family = indices.transfer_family;

        // Clean up temporary surface
        vkDestroySurfaceKHR(instance, temp_surface, allocator);

        if (graphics_queue_family == UINT32_MAX)
        {
            throw std::runtime_error("Failed to find graphics queue family");
        }

        // Query queue families for device creation
        uint32_t queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(
            selected_device, &queue_family_count, nullptr);
        std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(
            selected_device, &queue_family_count, queue_families.data());

        // Determine unique queue families
        std::set<uint32_t> unique_queue_families = { graphics_queue_family,
                                                     present_queue_family,
                                                     compute_queue_family,
                                                     transfer_queue_family };

        std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
        float                                queue_priority = 1.0f;

        for (uint32_t queue_family : unique_queue_families)
        {
            VkDeviceQueueCreateInfo queue_create_info{};
            queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_create_info.queueFamilyIndex = queue_family;
            queue_create_info.queueCount       = 1;
            queue_create_info.pQueuePriorities = &queue_priority;

            // If graphics and present use same family and we have enough queues, request
            // 2 queues
            if (queue_family == graphics_queue_family &&
                graphics_queue_family == present_queue_family &&
                queue_families[queue_family].queueCount >= 2)
            {
                queue_create_info.queueCount = 2;
            }

            queue_create_infos.push_back(queue_create_info);
        }

        // Device extensions
        std::vector<const char*> device_extensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME, // Basic requirement for rendering
            // Ray tracing extensions
            VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
            VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
            VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
            // Dynamic rendering
            VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME
        };

        VkDeviceCreateInfo device_create_info{};
        device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        device_create_info.pNext = &vulkan12_features; // Start of feature chain
        device_create_info.queueCreateInfoCount =
            static_cast<uint32_t>(queue_create_infos.size());
        device_create_info.pQueueCreateInfos = queue_create_infos.data();
        device_create_info.enabledLayerCount = 0;
        device_create_info.pEnabledFeatures  = &device_features;
        device_create_info.enabledExtensionCount =
            static_cast<uint32_t>(device_extensions.size());
        device_create_info.ppEnabledExtensionNames = device_extensions.data();

        VkResult result =
            vkCreateDevice(selected_device, &device_create_info, allocator, &device);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create logical device");
        }

        // Get queue handles
        vkGetDeviceQueue(device, graphics_queue_family, 0, &graphics_queue);

        // Get present queue - if same family as graphics and we got 2 queues, use index
        // 1, otherwise index 0
        uint32_t present_queue_index = 0;
        if (present_queue_family == graphics_queue_family &&
            queue_families[graphics_queue_family].queueCount >= 2)
        {
            present_queue_index = 1;
        }
        vkGetDeviceQueue(
            device, present_queue_family, present_queue_index, &present_queue);

        vkGetDeviceQueue(device, compute_queue_family, 0, &compute_queue);
        vkGetDeviceQueue(device, transfer_queue_family, 0, &transfer_queue);

        LOG_INFO(
            "Vulkan context initialized successfully with queues: Graphics({}) "
            "Present({}) Compute({}) Transfer({})",
            graphics_queue_family, present_queue_family, compute_queue_family,
            transfer_queue_family);
    }

    VulkanResources::~VulkanResources()
    {
        LOG_INFO("Cleaning up VulkanResources...");

        // Wait for device to be idle
        if (device != VK_NULL_HANDLE)
        {
            vkDeviceWaitIdle(device);
            vkDestroyDevice(device, allocator);
        }

        if (instance != VK_NULL_HANDLE)
        {
            vkDestroyInstance(instance, allocator);
        }

        LOG_INFO("VulkanResources cleanup complete");
    }
} // namespace v
