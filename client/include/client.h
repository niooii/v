//
// Created by niooi on 9/26/2025.
//

#pragma once

#include <atomic>
#include <engine/context.h>
#include <memory>
#include <prelude.h>

namespace v {
    class SDLContext;
    class WindowContext;
    class RenderContext;
    class NetworkContext;
    class NetConnection;
    class Window;

    class Client : public Context {
    public:
        explicit Client(Engine& engine);

        void update();

        bool is_running() const { return running_; }

    private:
        SDLContext*     sdl_ctx_;
        WindowContext*  window_ctx_;
        RenderContext*  render_ctx_;
        NetworkContext* net_ctx_;

        Window* window_;
        // Connection to a server
        std::shared_ptr<NetConnection> connection_;

        std::atomic<bool> running_;
    };
} // namespace v
