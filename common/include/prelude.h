//
// Created by niooi on 7/27/2025.
// WYSI!!
//

#pragma once

#include <defs.h>
#include <engine/components.h>
#include <engine/engine.h>
#include <engine/serial/compress.h>
#include <engine/serial/serde.h>
#include <entt/entt.hpp>

// engine subsystems
#include <rand.h>
#include <time/time.h>
#include <tracy/Tracy.hpp>

namespace spd = spdlog;

namespace v {
    void init(const char* argv0);
}
