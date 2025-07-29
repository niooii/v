//
// Created by niooi on 7/29/2025.
//

#pragma once

namespace v {
    class Context {
    public:
        Context() = default;

        // Contexts are non-copyable
        Context(const Context&)            = delete;
        Context& operator=(const Context&) = delete;
    };
}