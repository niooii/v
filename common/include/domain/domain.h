//
// Created by niooi on 7/28/2025.
//
// Inspired by:
// https://voxely.net/blog/object-oriented-entity-component-system-design/
// and https://voxely.net/blog/the-perfect-voxel-engine/
//

#pragma once

#include <defs.h>
#include <entt/entt.hpp>

namespace v {
    /// The base class for any Domain
    class Domain {
    public:
        Domain(class Engine& engine, std::string name);

        virtual ~Domain() = default;

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

} // namespace v
