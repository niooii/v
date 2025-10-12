//
// Created by niooi on 9/26/2025.
//

#include <client.h>
#include <engine/contexts/net/connection.h>
#include <engine/contexts/net/ctx.h>
#include <engine/contexts/render/ctx.h>
#include <engine/contexts/window/sdl.h>
#include <engine/contexts/window/window.h>
#include <net/channels.h>
#include <render/triangle_domain.h>
#include <world/world.h>
#include "engine/contexts/async/async.h"
#include "engine/contexts/render/default_render_domain.h"

namespace v {
    Client::Client(Engine& engine) : Context(engine), running_(true)
    {
        // all the contexts the client needs to functino
        sdl_ctx_    = engine_.add_ctx<SDLContext>();
        window_ctx_ = engine_.add_ctx<WindowContext>();
        window_     = window_ctx_->create_window("hjey man!", { 600, 600 }, { 600, 600 });

        // render_ctx_ = engine_.add_ctx<RenderContext>("./resources/shaders");
        net_ctx_    = engine_.add_ctx<NetworkContext>(1.0 / 500);
        // auto async_ctx = engine_.add_ctx<AsyncContext>();

        // Add triangle render domain
        // engine.add_domain<DefaultRenderDomain>();
        // engine_.add_domain<TriangleRenderDomain>();


        // Setup network connection
        LOG_INFO("Connecting to server...");
        connection_ = net_ctx_->create_connection("127.0.0.1", 25566);
        LOG_INFO("Connection created, attempting to connect...");

        // test channel kinda
        auto& channel = connection_->create_channel<ChatChannel>();
        auto& comp    = channel.create_component(engine_.entity());

        ChatMessage msg;
        msg.msg = "hi server man";
        channel.send(msg);

        // windows update task does not depend on anything
        // engine_.on_tick.connect({}, {}, "windows", [this]() { window_ctx_->update(); });

        // // render depends on windows task to be finished
        // engine_.on_tick.connect(
        //    { "windows" }, {}, "render", [this]() { render_ctx_->update(); });

        // network update task does not depend on anything
        engine_.on_tick.connect({}, {}, "network", [this]() { net_ctx_->update(); });

        constexpr u16 threads   = 16;
        auto*         async_ctx = engine_.add_ctx<AsyncContext>(threads);

        // async coroutine scheduler update
        engine_.on_tick.connect(
            {}, {}, "async_coro", [async_ctx]() { async_ctx->update(); });

        // handle the sdl quit event (includes keyboard interrupt)
        SDLComponent& sdl_comp = sdl_ctx_->create_component(engine_.entity());
        sdl_comp.on_quit       = [this]() { running_ = false; };

        // TODO! temporarily connect to the server with dummy info
        auto name = std::format("Player-{}", rand::irange(0, 1'000'000));
        LOG_INFO("Generated new random name {}", name);
        auto& connect_chnl = connection_->create_channel<ConnectServerChannel>();
        connect_chnl.send({ .uuid = name });
    }

    void Client::update()
    {
        sdl_ctx_->update();
        engine_.tick();
    }
} // namespace v
