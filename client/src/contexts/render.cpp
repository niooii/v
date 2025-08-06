//
// Created by niooi on 7/30/2025.
//

#include <contexts/render.h>
#include <VkBootstrap.h>
#include <stdexcept>

namespace v {
    RenderContext::RenderContext(Engine& engine) : Context(engine)
    {
        LOG_INFO("Initializing Vulkan context...");

        vkb::InstanceBuilder instance_builder;
        auto instance_ret = instance_builder
            .set_app_name("V Engine")
            .set_engine_name("V Engine")
            .require_api_version(1, 3, 0)
            .request_validation_layers(true)
            .use_default_debug_messenger()
            .build();

        if (!instance_ret) {
            LOG_ERROR("Failed to create Vulkan instance: {}", instance_ret.error().message());
            throw std::runtime_error("Failed to create Vulkan instance");
        }

        vkb::Instance vkb_instance = instance_ret.value();
        instance_ = vkb_instance.instance;
        LOG_DEBUG("Vulkan instance created successfully");

        VkPhysicalDeviceFeatures features{};
        VkPhysicalDeviceDynamicRenderingFeatures dynamic_rendering_features{};
        dynamic_rendering_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
        dynamic_rendering_features.dynamicRendering = VK_TRUE;

        vkb::PhysicalDeviceSelector selector{vkb_instance};
        auto physical_device_ret = selector
            .set_minimum_version(1, 3)
            .add_required_extension_features(dynamic_rendering_features)
            .select();

        if (!physical_device_ret) {
            LOG_ERROR("Failed to select physical device: {}", physical_device_ret.error().message());
            throw std::runtime_error("Failed to select physical device");
        }

        vkb::PhysicalDevice physical_device = physical_device_ret.value();
        LOG_DEBUG("Physical device selected: {}", physical_device.properties.deviceName);

        vkb::DeviceBuilder device_builder{physical_device};
        auto device_ret = device_builder.build();

        if (!device_ret) {
            LOG_ERROR("Failed to create logical device: {}", device_ret.error().message());
            throw std::runtime_error("Failed to create logical device");
        }

        vkb::Device vkb_device = device_ret.value();
        device_ = vkb_device.device;
        LOG_INFO("Vulkan context initialized successfully with dynamic rendering enabled");
    }

}