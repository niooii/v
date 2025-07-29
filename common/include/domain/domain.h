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
    class Engine;

    class Domain {
    public:
        Domain(Engine& engine, std::string name);

        virtual ~Domain();

        /// Override this method to add components TODO! is this necessary?
        virtual void init_standard_components() {}
        /// TODO!
        virtual void init_render_components(/*RenderContext* ctx*/) {}

        template <typename T, typename... Args>
        T& attach_component(Args&&... args);

        template <typename T>
        T* get_component();

        FORCEINLINE entt::entity get_entity() const { return entity_; }
        FORCEINLINE std::string_view get_name() const { return name_; }

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
