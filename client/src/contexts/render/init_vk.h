//
// Created by niooi on 8/7/25.
//

#pragma once

#include <defs.h>
#include <daxa/daxa.hpp>

namespace v {
    class Window;
    class WindowContext;
    class Engine;

    /// The daxa resources used globally
    struct DaxaResources {
        daxa::Instance instance;
        daxa::Device   device;

        explicit DaxaResources(Engine& engine);
        ~DaxaResources();

        DaxaResources(const DaxaResources&) = delete;
        DaxaResources& operator=(const DaxaResources&) = delete;
    };
}