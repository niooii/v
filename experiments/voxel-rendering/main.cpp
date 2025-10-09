//
// Created by niooi on 9/28/2025.
//

#include <engine/contexts/render/ctx.h>
#include <engine/contexts/window/sdl.h>
#include <engine/contexts/window/window.h>
#include <prelude.h>
#include "mesh.h"

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

    auto             sdl_comp = sdl_ctx->create_component(engine.entity());
    std::atomic_bool running  = true;
    sdl_comp.on_quit          = [&running]() { running = false; };

    // Main loop
    while (running)
    {
        sdl_ctx->update();
        window_ctx->update();
        render_ctx->update();
    }

    return 0;
}
