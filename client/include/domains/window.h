//
// Created by niooi on 7/29/2025.
//

#pragma once

#include <domain/domain.h>

namespace v {
    class WindowDomain : Domain {
    public:
        WindowDomain(Engine& engine, const std::string& name);
        WindowDomain(
            Engine& engine, const std::string& name, u16 w, u16 h);
    };
} // namespace v
