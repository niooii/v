//
// Created by niooi on 7/29/2025.
//

#pragma once

#include <engine/traits.h>
#include <memory>

namespace v {
    /// The base class for any Context to be attached to the Engine.
    ///
    /// IMPORTANT: When creating derived Context classes, the constructor MUST
    /// accept Engine& as its first parameter. The engine reference is automatically
    /// passed by Engine::add_ctx() - do not pass it manually when calling add_ctx().
    ///
    /// Example:
    ///   class MyContext : public Context<MyContext> {
    ///   public:
    ///       MyContext(Engine& engine, double update_rate) : Context(engine) { ... }
    ///   };
    ///
    ///   // Correct usage:
    ///   engine.add_ctx<MyContext>(1.0 / 60.0);  // Engine& is passed automatically
    ///
    ///   // WRONG - will cause compile error:
    ///   engine.add_ctx<MyContext>(engine, 1.0 / 60.0);  // Don't pass engine manually!
    template <typename Derived>
    class Context : public QueryBy<std::unique_ptr<Derived>> {
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
