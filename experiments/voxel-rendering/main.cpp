//
// Created by niooi on 9/28/2025.
//

#include <engine/contexts/render/ctx.h>
#include <engine/contexts/window/sdl.h>
#include <engine/contexts/window/window.h>
#include <prelude.h>
#include "engine/contexts/render/default_render_domain.h"
#include "mesh.h"
#include "render/triangle_domain.h"

using namespace v;

int main(int argc, char** argv)
{
    v::init(argv[0]);

    Engine engine{};

    // Setup SDL and window contexts
    auto sdl_ctx    = engine.add_ctx<SDLContext>();
    auto window_ctx = engine.add_ctx<WindowContext>();
    auto window =
        window_ctx->create_window("Voxel Rendering", { 800, 600 }, { 100, 100 });

    // Setup render context (includes daxa)
    auto render_ctx = engine.add_ctx<RenderContext>();

    // Test DefaultRenderDomain via framework
    // needs at least a default domain to show window on some systems
    engine.add_domain<DefaultRenderDomain>();

    std::atomic_bool running = true;
    sdl_ctx->quit().connect([&running]() { running = false; });

    window->capture_mouse(true);

    // Main loop
    while (running)
    {
        window_ctx->update();
        sdl_ctx->update();
        render_ctx->update();

        if (window->is_key_down(Key::W))
            spdlog::info("W down");
        if (window->is_key_down(Key::A))
            spdlog::info("A down");
        if (window->is_key_down(Key::S))
            spdlog::info("S down");
        if (window->is_key_down(Key::D))
            spdlog::info("D down");
        if (window->is_key_down(Key::Q))
            spdlog::info("Q down");
        if (window->is_key_down(Key::E))
            spdlog::info("E down");

        if (window->is_key_down(Key::Escape))
            running = false;

        glm::ivec2 mouse_delta = window->get_mouse_delta();
        if (mouse_delta.x != 0 || mouse_delta.y != 0)
            spdlog::info("Mouse delta: ({}, {})", mouse_delta.x, mouse_delta.y);

        engine.tick();
    }

    return 0;
}
