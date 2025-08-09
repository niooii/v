//
// Created by niooi on 7/28/2025.
//

#pragma once

#include <defs.h>
#include <domain/context.h>
#include <domain/domain.h>
#include <engine/sync.h>
#include <entt/entt.hpp>
#include <moodycamel/concurrentqueue.h>
#include <shared_mutex>
#include <time/stopwatch.h>
#include <unordered_dense.h>

#include "sink.h"

template <typename T>
concept DerivedFromDomain = std::is_base_of_v<v::Domain, T>;

template <typename T>
concept DerivedFromContext = std::is_base_of_v<v::Context, T>;

namespace v {

    class Engine {
    public:
        // Engine is non-copyable
        Engine(const Engine&)            = delete;
        Engine& operator=(const Engine&) = delete;

        Engine();
        ~Engine();

        /// Processes queued actions and updates deltatime.
        /// @note Should be called first in a main loop
        void tick();

        /// Returns the deltaTime (time between previous tick start and current tick
        /// start) in seconds. This will return 0 on the first frame.
        /// @note This is not thread safe, but can be safely called from multiple threads
        /// as long as it does not overlap with the Engine::tick() method
        FORCEINLINE f64 delta_time() const { return prev_tick_span_; };

        /// Returns the internally stored tick counter
        FORCEINLINE u64 current_tick() const { return current_tick_; };

        /// Return's the engine's reserved entity in the main registry (cannot query
        /// contexts from this registry)
        FORCEINLINE entt::entity entity() const { return engine_entity_; };

        /// Fetch the domain registry (ecs entity registry) with a scoped read lock.
        /// @note The lock is dropped after the value returned goes out of scope, or a
        /// call to guard.release(). The registry and any components from the registry
        /// must not be accessed after a call to guard.release()
        ReadProtectedResourceGuard<entt::registry> registry_read() const;

        /// Fetch the domain registry (ecs entity registry) with a scoped write lock.
        /// @note The lock is dropped after the value returned goes out of scope, or a
        /// call to guard.release(). The registry and any components from the registry
        /// must not be accessed after a call to guard.release()
        WriteProtectedResourceGuard<entt::registry> registry_write();

        /// Adds a context to the engine and wraps it in a RWLock.
        /// Retrievable by original type.
        /// Unless the Context implementation is thread-safe or will only be used
        /// on one thread, you almost always want to use this over Engine::add_context.
        /// @note Contexts must be added before the program starts running
        /// multiple threads - adding Contexts after is undefined behavior.
        /// This means Contexts should be added during the application's
        /// initialization.
        template <DerivedFromContext T, typename... Args>
        RWProtectedResource<T>* add_locked_context(Args&&... args)
        {
            return _add_context<RWProtectedResource<T>>(std::forward<Args>(args)...);
        }

        /// Retrieve a read lock for a locked context by type
        template <DerivedFromContext T>
        FORCEINLINE std::optional<ReadProtectedResourceGuard<T>> read_context() const
        {
            using namespace std;
            if (auto resource = ctx_registry_.try_get<unique_ptr<RWProtectedResource<T>>>(
                    ctx_entity_))
                return std::optional{ (*resource)->read() };

            return std::nullopt;
        }

        /// Retrieve a write lock for a locked context by type
        template <DerivedFromContext T>
        FORCEINLINE std::optional<WriteProtectedResourceGuard<T>> write_context()
        {
            using namespace std;
            if (auto resource = ctx_registry_.try_get<unique_ptr<RWProtectedResource<T>>>(
                    ctx_entity_))
                return std::optional{ (*resource)->write() };

            return std::nullopt;
        }

        /// Adds a context to the engine, retrievable by type.
        /// Cannot be retrieved using the Engine::read_context and Engine::write_context
        /// methods, as those are for locked Contexts.
        /// Use Engine::get_context instead.
        /// The Context implementation should be thread-safe or should not be
        /// used on multiple threads, as this method does not provide locks.
        /// @note Contexts must be added before the program starts running
        /// multiple threads - adding Contexts after is undefined behavior.
        /// This means Contexts should be added during the application's
        /// initialization.
        template <DerivedFromContext T, typename... Args>
        T* add_context(Args&&... args)
        {
            return _add_context<T>(std::forward<Args>(args)...);
        }

        /// Retrieve a context added with Engine::add_context by type
        template <DerivedFromContext T>
        T* get_context()
        {
            if (auto context = ctx_registry_.try_get<std::unique_ptr<T>>(ctx_entity_))
                return context->get();

            return nullptr;
        }

        /// Create a new domain owned by the Engine, retrievable using
        /// the registry and accessible by the type pointer.
        /// e.g. engine.registry_read()->view<T*>().
        /// @note Pointers to Domains may be stored, as they are heap allocated.
        /// Though pointers to Domains are stable, they are **not** thread safe, and
        /// the programmer must handle concurrent access to domains themselves (through
        /// either inheriting from the ConcurrentDomain class and acquiring RW locks, or
        /// manual synchronization). Pointer stability is guaranteed UNTIL
        /// Engine::queue_destroy_domain has been called on it. After that, pointers are
        /// no longer guaranteed to exist
        template <DerivedFromDomain T, typename... Args>
        T* add_domain(Args&&... args)
        {

            std::unique_ptr<T> domain = std::make_unique<T>(std::forward<Args>(args)...);
            T*                 ptr    = domain.get();

            ptr->init_standard_components();
            // TODO!
            // ptr->init_render_components();

            // Attach the domain (ptr) itself to its own entity
            // After careful consideration i have decided its more worth
            // to store the pointer even though it leads to more cache misses,
            // since domains won't usually be small enough for it to matter anyway
            auto reg = registry_write();
            reg->emplace<T*>(ptr->entity(), ptr);

            // Attach the actual unique ptr to itself for lifetime management
            auto& elem =
                reg->emplace<std::unique_ptr<Domain>>(ptr->entity(), std::move(domain));

            return ptr;
        }

        /// Queues a domain to be destroyed on the next tick.
        /// @note Any pointers to a domain with the id domain_id are no longer safe
        FORCEINLINE void queue_destroy_domain(const entt::entity domain_id)
        {
            domain_destroy_queue_.enqueue(domain_id);
        }

        /// Runs every time Engine::tick is called
        RWProtectedResource<DependentSink> on_tick;

        /// Runs within the Engine's destructor, before domains and contexts are destroyed
        RWProtectedResource<DependentSink> on_destroy;

    private:
        /// A central registry to store domains
        RWProtectedResource<entt::registry> registry_{};

        /// An internal registry for the engine's contexts
        entt::registry ctx_registry_{};

        /// A queue for the destruction of domains
        moodycamel::ConcurrentQueue<entt::entity> domain_destroy_queue_{};

        /// The engine's private entity for storing contexts. This allows us to have
        /// 'contexts' (essentially singleton components) that we can fetch
        /// by type
        entt::entity ctx_entity_;

        /// The engine's entity from the domain registry.  This allows us to create
        /// components from some contexts that are attached to the Engine itself (e.g.
        /// from a main() function)
        entt::entity engine_entity_;

        Stopwatch tick_time_stopwatch_{};
        /// How long it took between the previous tick's start and the current tick's
        /// start. In other words, the 'deltaTime' variable
        f64 prev_tick_span_{ 0 };
        u64 current_tick_{ 0 };

        template <typename T, typename... Args>
        T* _add_context(Args&&... args)
        {
            using namespace std;

            // If the component already exists, remove it
            if (ctx_registry_.all_of<unique_ptr<T>>(ctx_entity_))
            {
                LOG_WARN("Adding duplicate context, removing old instance..");
                ctx_registry_.remove<unique_ptr<T>>(ctx_entity_);
            }

            auto context = make_unique<T>(forward<Args>(args)...);

            T* ptr = context.get();

            ctx_registry_.emplace<std::unique_ptr<T>>(ctx_entity_, std::move(context));

            return ptr;
        }
    };
} // namespace v
