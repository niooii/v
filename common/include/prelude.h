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

// expose the rendering stuff and other conexts
#include <engine/contexts/render/render_domain.h>

namespace spd = spdlog;

namespace v {
    void init(const char* argv0);
}
