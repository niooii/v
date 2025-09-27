//
// Created by niooi on 7/28/2025.
//

#pragma once

#include <defs.h>
#include <domain/context.h>
#include <domain/domain.h>
#include <entt/entt.hpp>
#include <functional>
#include <moodycamel/concurrentqueue.h>
#include <time/stopwatch.h>
#include <unordered_dense.h>

#include "entt/entity/fwd.hpp"
#include "sink.h"
#include "traits.h"

template <typename T>
concept DerivedFromDomain = std::is_base_of_v<v::Domain<T>, T>;

template <typename T>
concept DerivedFromContext = std::is_base_of_v<v::Context, T>;

template <typename T>
concept DerivedFromSDomain = std::is_base_of_v<v::SDomain<T>, T>;

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

        /// Get a reference to the domain registry (ecs entity registry).
        FORCEINLINE entt::registry& registry() { return registry_; }

        /// Enqueue a callback to run right after this frame's on_tick callbacks.
        /// Multiple threads may call post_tick; execution happens
        /// on the main thread within Engine::tick().
        void post_tick(std::function<void()> fn)
        {
            post_tick_queue_.enqueue(std::move(fn));
        }


        /// Get the first domain of type T, returns nullptr if none exist
        template <DerivedFromDomain T>
        T* get_domain()
        {
            auto view = registry_.view<query_transform_t<T>>();
            if (view.empty())
                return nullptr;
            return view.template get<query_transform_t<T>>(*view.begin()).get();
        }

        /// Try to get domain T from entity, returns nullptr if not found
        template <DerivedFromDomain T>
        FORCEINLINE T* try_get_domain(entt::entity entity)
        {
            auto ptr = registry_.try_get<query_transform_t<T>>(entity);
            return ptr ? ptr->get() : nullptr;
        }

        /// Directly query the entt registry
        template <typename... Components>
        FORCEINLINE auto raw_view()
        {
            return registry_.view<Components...>();
        }

        /// Queries for components from the main engine registry.
        /// Types that inherit from QueryBy are automatically transformed
        /// into their specified query types.
        ///
        /// Ex.
        /// engine.view<PlayerDomain, PoisonComponent>();
        /// works along with
        /// engine.raw_view<std::unique_ptr<PlayerDomain>, PoisonComponent>();
        /// because all Domains inherit from QueryBy<std::unique_ptr<Derived>>
        template <typename... Types>
        FORCEINLINE auto view()
        {
            return registry_.view<query_transform_t<Types>...>();
        }

        /// Check if entity has component T
        template <typename T>
        FORCEINLINE bool has_component(entt::entity entity)
        {
            return registry_.all_of<T>(entity);
        }

        /// Try to get component T from entity, returns nullptr if not found
        template <typename T>
        FORCEINLINE T* try_get_component(entt::entity entity)
        {
            return registry_.try_get<T>(entity);
        }

        /// Add/replace component T to entity
        template <typename T, typename... Args>
        FORCEINLINE T& add_component(entt::entity entity, Args&&... args)
        {
            return registry_.emplace_or_replace<T>(entity, std::forward<Args>(args)...);
        }

        /// Check if entity is valid
        FORCEINLINE bool is_valid_entity(entt::entity entity)
        {
            return registry_.valid(entity);
        }

        /// Get component T from entity, throws if component doesn't exist
        template <typename T>
        FORCEINLINE T& get_component(entt::entity entity)
        {
            return registry_.get<T>(entity);
        }

        /// Remove component T from entity, returns number of components removed
        template <typename T>
        FORCEINLINE std::size_t remove_component(entt::entity entity)
        {
            return registry_.remove<T>(entity);
        }

        /// Adds a context to the engine, retrievable by type.
        /// @note Single-threaded engine - contexts are not thread-safe.
        template <DerivedFromContext T, typename... Args>
        T* add_ctx(Args&&... args)
        {
            return _add_context<T>(std::forward<Args>(args)...);
        }

        /// Retrieve a context by type
        template <DerivedFromContext T>
        T* get_ctx()
        {
            if (auto context = ctx_registry_.try_get<std::unique_ptr<T>>(ctx_entity_))
                return context->get();

            return nullptr;
        }

        /// Create a new domain owned by the Engine, retrievable using
        /// domain convenience methods.
        /// e.g. engine.view<T>() or engine.get_domain<T>().
        /// @note Pointers to Domains may be stored, as they are heap allocated.
        /// Pointer stability is guaranteed UNTIL Engine::queue_destroy_domain
        /// has been called on it. After that, pointers are no longer guaranteed to exist.
        /// Single-threaded engine - domains are not thread-safe.
        template <DerivedFromDomain T, typename... Args>
        T* add_domain(Args&&... args)
        {
            // Check if this is a singleton domain and if it already exists
            if constexpr (DerivedFromSDomain<T>)
            {
                if (auto existing = get_domain<T>())
                {
                    LOG_WARN(
                        "Singleton domain {} already exists, returning existing instance",
                        typeid(T).name());
                    return existing;
                }
            }

            std::unique_ptr<T> domain = std::make_unique<T>(std::forward<Args>(args)...);
            T*                 ptr    = domain.get();

            ptr->init_standard_components();
            // TODO!
            // ptr->init_render_components();

            // Attach the unique ptr for lifetime management
            registry_.emplace<query_transform_t<T>>(ptr->entity(), std::move(domain));

            return ptr;
        }

        FORCEINLINE void queue_destroy_domain(const entt::entity domain_id)
        {
            post_tick(
                [this, domain_id]()
                {
                    if (registry_.valid(domain_id))
                        registry_.destroy(domain_id);
                });
        }

        /// Runs every time Engine::tick is called
        DependentSink on_tick;

        /// Runs within the Engine's destructor, before domains and contexts are destroyed
        DependentSink on_destroy;

    private:
        /// TODO! this better be destroyed last otherwise stupid bad
        /// race condition potentially.
        /// Since objects are destroyed in reverse order of declaration,
        /// this should be consistent.

        /// An internal registry for the engine's contexts
        entt::registry ctx_registry_{};

        /// A central registry to store domains
        entt::registry registry_{};

        /// A queue for deferred work to run after each tick()
        moodycamel::ConcurrentQueue<std::function<void()>> post_tick_queue_{};

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
            if (ctx_registry_.all_of<std::unique_ptr<T>>(ctx_entity_))
            {
                LOG_WARN("Adding duplicate context, removing old instance..");
                ctx_registry_.remove<std::unique_ptr<T>>(ctx_entity_);
            }

            auto context = std::make_unique<T>(std::forward<Args>(args)...);

            T* ptr = context.get();

            ctx_registry_.emplace<std::unique_ptr<T>>(ctx_entity_, std::move(context));

            return ptr;
        }
    };
} // namespace v
