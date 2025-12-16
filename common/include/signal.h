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
    template <typename T>
    class Signal;

    class SignalConnection {
        template <typename T>
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

        u32                      id_;
        bool                     dcd_;
        std::function<bool(u32)> dc_fn;
    };

    template <typename T>
    class Signal {
    public:
        /// Connect a callback to run when the signal fires.
        /// The callback will always run on the main thread.
        virtual SignalConnection connect(std::function<void(T)> func)
        {
            connections_.push_back(func);

            // TODO! need some sort of invalidation flag, or to guarentee that the Signal
            // outlives the SignalConnection MOST LIKELY we will just shove a Rc<void> in
            // here with the same control block, void so we don't need to make the
            // SignalConnection class a template class
            // TODO! also yea this is just really shaky idk how the memory stuff works
            // too lua rotted rn
            return SignalConnection(connections_.size() - 1, this->disconnect_id);
        }

        // TODO! not finished yet
        struct SigAwaitable {
            bool done = false;

            bool await_ready() const noexcept { return done; }

            void await_suspend(std::coroutine_handle<> handle);

            /// Return the usual values that the signal fires
            T await_resume() const noexcept { return true; }
        };

        /// Halts the current coroutine until the signal fires
        SigAwaitable await();

    protected:
        Signal() = default;

        std::vector<std::function<void(T)>> connections_{};

        // internal util for firing
        virtual void fire(T&& val)
        {
            for (auto& fn : connections_)
                fn(val);
        }

        // internal util for disconncting
        bool disconnect_id(u32 id)
        {
            if (id >= connections_.size())
                return false;
            // we trust the connection object validates it
            // for us
            // swap end and delete end
            connections_[id] = std::move(connections_.back());
            connections_.pop_back();
        }
    };

    template <typename T>
    class ThreadSafeSignal : public Signal<T> {
    public:
        virtual SignalConnection connect(std::function<void(T)> func)
        {
            conn_lock_.Lock();
            SignalConnection conn = Signal<T>::connect(func);
            conn_lock_.Unlock();

            return conn;
        }

        /// Halts the execution of the current thread until the signal is fired
        T wait()
        {
            u64 curr = gen_c_;

            wait_.Await(
                absl::Condition(
                    +[&](ThreadSafeSignal* s) { return s->gen_c_ != gen_c_; }, this));

            // TODO! could be diabolical race conditions here
            // (vanishing payload/duplicate check later)
            return most_recent_payload_;
        }

    private:
        ThreadSafeSignal(Engine& engine) : engine_(engine) {

        }

        Engine& engine_;

        absl::Mutex conn_lock_{};

        absl::Mutex wait_{};
        u64 gen_c_  ABSL_GUARDED_BY(wait_);
        T           most_recent_payload_;

        bool disconnect_id(u32 id) override
        {
            conn_lock_.Lock();
            bool res = Signal<T>::disconnect_id(id);
            conn_lock_.Unlock();

            return res;
        }

        void fire(T&& val) override
        {
            // first fire waiters for the
            // wait function (all waiters should resume in parallel, since we're on
            // multiple threads)
            {
                absl::MutexLock l(&wait_);
                gen_c_++;
                // might have to do some waiter queue shenanigans or remove themselves
                // from a map idk gg AHHH
                most_recent_payload_ = val;
            }

            engine_.post_tick(
                [this, v = std::move(val)]()
                {
                    conn_lock_.Lock();
                    Signal<T>::fire(std::move(v));
                    conn_lock_.Unlock();
                });
        }
    };

    /// An event type with an associated signal. Not thread safe
    template <typename T>
    class Event {
        using sigtype = Signal<T>;

    public:
        Event() = default;
        
        /// The signal associated with the event
        FORCEINLINE mem::Rc<sigtype> signal() { return signal_; }

        /// Fire the signal associated with the event
        FORCEINLINE void fire(T&& val) { signal_->fire(val); }

    private:
        mem::Rc<sigtype> signal_{};
    };

    /// An event type that can be used safely between multiple threads
    template <typename T>
    class ThreadSafeEvent {
        using sigtype = ThreadSafeSignal<T>;

    public:
        ThreadSafeEvent(Engine& engine) : signal_(engine) {
            
        }

        /// The signal associated with the event
        FORCEINLINE mem::Arc<sigtype> signal() { return signal_; }

        /// Fire the signal associated with the event
        FORCEINLINE void fire(T&& val) { signal_->fire(val); }

    private:
        mem::Rc<sigtype> signal_;
    };
} // namespace v
