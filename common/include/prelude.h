//
// Created by niooi on 7/27/2025.
//

#pragma once

#include <defs.h>
#include <engine/engine.h>
#include <engine/serial/serde.h>
#include <engine/serial/compress.h>
#include <entt/entt.hpp>

namespace spd = spdlog;

namespace v {
    void init(const char* argv0);
}
