//
// Created by niooi on 9/26/2025.
//

#include <client.h>
#include <engine/contexts/net/connection.h>
#include <engine/contexts/net/ctx.h>
#include <engine/contexts/window/sdl.h>
#include <engine/contexts/window/window.h>
#include <net/channels.h>
#include <render/ctx.h>
#include <world/world.h>

namespace v {
    Client::Client(Engine& engine) : Context(engine), running_(true)
    {
        // all the contexts the client needs to functino
        sdl_ctx_    = engine_.add_ctx<SDLContext>(engine_);
        window_ctx_ = engine_.add_ctx<WindowContext>(engine_);
        render_ctx_ = engine_.add_ctx<RenderContext>(engine_);
        net_ctx_    = engine_.add_ctx<NetworkContext>(engine_, 1.0 / 500);

        window_ = window_ctx_->create_window("hjey man!", { 600, 600 }, { 600, 600 });

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
        engine_.on_tick.connect(
            {}, {}, "windows",
            [this]()
            {
                sdl_ctx_->update();
                window_ctx_->update();
            });

        // render depends on windows task to be finished
        engine_.on_tick.connect(
            { "windows" }, {}, "render", [this]() { render_ctx_->update(); });

        // network update task does not depend on anything
        engine_.on_tick.connect({}, {}, "network", [this]() { net_ctx_->update(); });

        // handle the sdl quit event (includes keyboard interrupt)
        SDLComponent& sdl_comp = sdl_ctx_->create_component(engine_.entity());
        sdl_comp.on_quit       = [this]() { running_ = false; };
    }

    void Client::update() { engine_.tick(); }
} // namespace v
