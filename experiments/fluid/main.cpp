//
// Created by niooi on 9/28/2025.
//

#include <engine/contexts/render/ctx.h>
#include <engine/contexts/window/sdl.h>
#include <engine/contexts/window/window.h>
#include <prelude.h>
#include <util/devcam.h>
#include "engine/contexts/render/default_render_domain.h"
#include "render/triangle_domain.h"

using namespace v;

int main(int argc, char** argv)
{
    v::init(argv[0]);

    Engine engine{};

    auto sdl_ctx    = engine.add_ctx<SDLContext>();
    auto window_ctx = engine.add_ctx<WindowContext>();
    auto window = window_ctx->create_window("Fluid stuff", { 800, 600 }, { 100, 100 });

    auto render_ctx = engine.add_ctx<RenderContext>();

    engine.add_domain<DefaultRenderDomain>();
    auto& devcam = engine.add_domain<DevCamera>();

    auto             sdl_comp = sdl_ctx->create_component(engine.entity());
    std::atomic_bool running  = true;
    sdl_comp.on_quit          = [&running]() { running = false; };

    window->capture_mouse(true);

    // Main loop
    while (running)
    {
        window_ctx->update();
        sdl_ctx->update();
        render_ctx->update();

        engine.tick();
    }

    return 0;
}
