//
// Created by niooi on 12/15/2025.
//

#pragma once

#include <defs.h>
#include <engine/engine.h>
#include <mem/ref_counted.h>

#include <absl/synchronization/mutex.h>

#include <coroutine>
#include <functional>
#include <tuple>
#include <vector>

namespace v {
    template <typename... Args>
    class Signal;

    class SignalConnection {
        template<typename ...Args>
        friend class Signal;

    public:
        // Delete copy operations
        // The Connection object itself tracks whether
        // it has been disconnected or not, so we must
        // prevent copies to prevent disconnecting the
        // wrong connection (see how disconnecting is implemented below)
        SignalConnection(const SignalConnection&)            = delete;
        SignalConnection& operator=(const SignalConnection&) = delete;

        // Move constructor and assignment
        SignalConnection(SignalConnection&&)            = default;
        SignalConnection& operator=(SignalConnection&&) = default;

        /// Succeeds if it was active, fails if
        /// it was already disconnected
        FORCEINLINE bool disconnect()
        {
            if (LIKELY(!dcd_))
            {
                bool res = dc_fn(id_);

                if (LIKELY(res))
                {
                    dcd_ = true;
                    return true;
                }
            }
            return false;
        }

    private:
        SignalConnection(u32 id, std::function<bool(u32)> disconnect_fn) :
            id_(id), dc_fn(disconnect_fn), dcd_(false)
        {}

        u32  id_;
        bool dcd_;
        std::function<bool(u32)> dc_fn;
    };

    template <typename... Args>
    class Signal {
    public:
        virtual SignalConnection connect(std::function<void(Args...)> func) {
            connections_.push_back(func);

            // TODO! need some sort of invalidation flag, or to guarentee that the Signal outlives the SignalConnection
            // MOST LIKELY we will just shove a 
            // Rc<void> in here with the same control block,
            // void so we don't need to make the SignalConnection class a template class
            return SignalConnection(connections_.size() - 1, this->disconnect_id);
        }

        struct SigAwaitable {
            bool done = false;

            bool await_ready() const noexcept { return done; }

            void await_suspend(std::coroutine_handle<> handle);

            /// Return the usual values that the signal fires
            std::tuple<Args...> await_resume() const noexcept { return true; }
        };

        /// Halts the current coroutine until the signal fires
        SigAwaitable await();

    protected:
        std::vector<std::function<void(Args...)>> connections_{};

        virtual void fire(Args&&... args)
        {
            for (auto& fn : connections_)
                fn(args...);
        }

        bool disconnect_id(u32 id)
        {
            if (id >= connections_.size())
                return false;
            // we trust the connection object validates it
            // for us
            // swap end and delete end
            connections_[id] = connections_.pop_back();
        }
    };

    template <typename... Args>
    class ThreadSafeSignal : public Signal<Args...> {
    public:
        virtual SignalConnection connect(std::function<void(Args...)> func) {
            conn_lock_.Lock();
            SignalConnection conn = Signal<Args...>::connect(func);
            conn_lock_.Unlock();

            return conn;
        }

        std::tuple<Args...> wait()
        {
            //
            // TODO! so how will this be done..
            // we need osme sort of event thingy
        }

    private:
        Engine& engine_;

        absl::Mutex conn_lock_{};

        bool disconnect_id(u32 id) override
        {
            conn_lock_.Lock();
            bool res = Signal<Args...>::disconnect_id(id);
            conn_lock_.Unlock();

            return res;
        }

        void fire(Args&&... args) override
        {
            // TODO! first fire waiters for the
            // wait function (all waiters should resume in parallel, since we're on
            // multiple threads)

            auto fire_fn = std::bind(Signal<Args...>::fire, args...);
            engine_.post_tick(
                [&]()
                {
                    conn_lock_.Lock();
                    fire_fn();
                    conn_lock_.Unlock();
                });
        }
    };

    template<typename ...Args>
    class Event {
    using sigtype = Signal<Args...>;
    public:
        FORCEINLINE mem::Rc<sigtype> signal() {
            return signal_;
        }

        FORCEINLINE void fire(Args&&... args) {
            signal_->fire(args...);
        }

    private:
        mem::Rc<sigtype> signal_;
    };

    template<typename ...Args>
    class ThreadSafeEvent {
    using sigtype = ThreadSafeSignal<Args...>;
    public:
        FORCEINLINE mem::Arc<sigtype> signal() {
            return signal_;
        }

        FORCEINLINE void fire(Args&&... args) {
            signal_->fire(args...);
        }

    private:
        mem::Rc<sigtype> signal_;
    };
} // namespace v
