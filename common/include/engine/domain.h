//
// Created by niooi on 7/28/2025.
//
// Inspired by:
// https://voxely.net/blog/object-oriented-entity-component-system-design/
// and https://voxely.net/blog/the-perfect-voxel-engine/
//

#pragma once

#include <defs.h>
#include <engine/traits.h>
#include <entt/entt.hpp>
#include <memory>

namespace v {
    class Engine;

    /// Base class for all domains.
    ///
    /// IMPORTANT: When creating derived Domain classes, the constructor MUST
    /// accept Engine& as its first parameter. The engine reference is automatically
    /// passed by Engine::add_domain() - do not pass it manually when calling add_domain().
    ///
    /// Example:
    ///   class MyDomain : public Domain<MyDomain> {
    ///   public:
    ///       MyDomain(Engine& engine, int my_param) : Domain(engine) { ... }
    ///   };
    ///
    ///   // Correct usage:
    ///   engine.add_domain<MyDomain>(42);  // Engine& is passed automatically
    ///
    ///   // WRONG - will cause compile error:
    ///   engine.add_domain<MyDomain>(engine, 42);  // Don't pass engine manually!
    class DomainBase {
    public:
        DomainBase(Engine& engine, std::string name);
        virtual ~DomainBase();

        /// Override this method to add components TODO! is this necessary?
        virtual void init_standard_components() {}
        /// TODO!
        virtual void init_render_components(/*RenderContext* ctx*/) {}

        FORCEINLINE entt::entity entity() const { return entity_; }
        FORCEINLINE std::string_view name() const { return name_; }

    protected:
        /// A reference to the engine instance
        Engine& engine_;
        /// The name of the domain
        std::string name_;
        /// The entity this domain is registered as in the engine's
        /// registry
        entt::entity entity_;
    };

    template <typename Derived>
    class Domain : public DomainBase, public QueryBy<std::unique_ptr<Derived>> {
    public:
        Domain(Engine& engine, std::string name = std::string{ type_name<Derived>() }) :
            DomainBase(engine, std::move(name))
        {}

        virtual ~Domain() = default;
    };

    /// A singleton domain which does not permit the creation of multiple
    /// instances of itself in the same Engine container.
    template <typename Derived>
    class SDomain : public Domain<Derived> {
    public:
        SDomain(
            Engine&            engine,
            const std::string& name = std::string{ type_name<Derived>() }) :
            Domain<Derived>(engine, name)
        {}
    };
} // namespace v
