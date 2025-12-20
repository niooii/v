//
// Created by niooi on 12/15/2025.
//

#pragma once

#include <defs.h>
#include <memory>

// TODO!
namespace v::mem {
    /// A reference counted smart pointer, not atomic to avoid unneeded overhead
    template <typename T>
    class Rc {
    public:
        // actual constructors
        template <typename... Args>
        Rc(Args&&... args);
        // TODO add a value constructor and ref and double ref (move)

        // copy constructors
        Rc(const Rc&) { blk_->refc++; };
        Rc& operator=(const Rc&) { blk_->refc++; };

        // move constructors
        Rc(Rc&&)            = default;
        Rc& operator=(Rc&&) = default;

        ~Rc()
        {
            blk_->refc--;

            if (!blk_->refc)
                delete ptr_;
        }

    private:
        struct ControlBlock {
            u64 refc;
        }* blk_;

        T* ptr_;
    };

    /// An atomically reference counted pointer, akin to std::shared_ptr
    /// haha actually its just std::shared ptr!
    template <typename T>
    class Arc {
    public:
    private:
        std::shared_ptr<T> ptr_;
    };
} // namespace v::mem

// aliases for now
namespace v {
    template <typename T>
    using Rc = v::mem::Rc<T>;

    template <typename T>
    using Arc = v::mem::Rc<T>;
} // namespace v
