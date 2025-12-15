//
// Created by niooi on 12/15/2025.
//

#pragma once

#include <defs.h>
#include <memory>

// TODO!

namespace v::mem {
    /// A reference counted smart pointer, not atomic to avoid unneeded overhead
    /// TODO! also what is an intrusive pointer lol that sounds useufl
    template <typename T>
    class Rc {
    public:
    private:
        struct ControlBlock {
            u64 refc_;
        } blk_;

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
