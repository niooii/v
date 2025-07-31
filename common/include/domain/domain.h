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
#include <shared_mutex>

namespace v {
    /// The base class for any Domain
    class Domain {
    public:
        Domain(class Engine& engine, std::string name);

        virtual ~Domain();

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

    /// A simple Domain wrapper with a shared_mutex.
    /// The synchronization should be handled by the programmer, this class
    /// does not do anything special besides provide read/write lock methods.
    /// This is only significant for components stored in the ECS as *pointers*,
    /// since a const registry would return const refs to components anyway.
    /// However, the current Engine design will store Domains as pointers, so
    /// if concurrent read access is needed for a Domain, do inherit from this class
    class ConcurrentDomain : public Domain /* expansion */ {
    public:
        ConcurrentDomain(Engine& engine, const std::string& name) :
            Domain(engine, name) {};

        FORCEINLINE std::unique_lock<std::shared_mutex> write_lock()
        {
            return std::unique_lock{ shared_mutex_ };
        }

        FORCEINLINE std::shared_lock<std::shared_mutex> read_lock()
        {
            return std::shared_lock{ shared_mutex_ };
        }

        FORCEINLINE std::shared_mutex& mutex() { return shared_mutex_; }

    private:
        std::shared_mutex shared_mutex_;
    };
} // namespace v
