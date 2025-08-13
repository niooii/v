//
// Created by niooi on 8/7/25.
//

#pragma once

#include <daxa/daxa.hpp>
#include <daxa/utils/pipeline_manager.hpp>
#include <defs.h>

namespace v {
    class Window;
    class WindowContext;
    class Engine;

    /// The daxa resources used globally
    struct DaxaResources {
        daxa::Instance        instance;
        daxa::Device          device;
        daxa::PipelineManager pipeline_manager;

        explicit DaxaResources(Engine& engine);
        ~DaxaResources();

        DaxaResources(const DaxaResources&)            = delete;
        DaxaResources& operator=(const DaxaResources&) = delete;
    };
} // namespace v

