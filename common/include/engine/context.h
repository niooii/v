//
// Created by niooi on 7/29/2025.
//

#pragma once

namespace v {
    /// The base class for any Context to be attached to the Engine.
    class Context {
        friend class Engine;

    public:
        explicit Context(class Engine& engine) : engine_{ engine } {};

        // Contexts are non-copyable
        Context(const Context&)            = delete;
        Context& operator=(const Context&) = delete;

    protected:
        Engine& engine_;
    };
} // namespace v
