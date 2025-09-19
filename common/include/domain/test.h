//
// Created by niooi on 7/31/2025.
//

#pragma once

#include <domain/domain.h>

namespace v {
    class CountTo10Domain : public Domain<CountTo10Domain> {
    public:
        CountTo10Domain(Engine& engine, const std::string& name);
        ~CountTo10Domain() override { LOG_INFO("Destroying CountTo10Domain."); };

        void update();

    private:
        i32 counter_;
    };
} // namespace v
