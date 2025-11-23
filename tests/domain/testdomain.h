//
// Created by niooi on 11/23/2025.
//

#pragma once

#include <engine/domain.h>

namespace v {
    class CountTo10Domain : public Domain<CountTo10Domain> {
    public:
        CountTo10Domain(const std::string& name);
        ~CountTo10Domain() override { LOG_TRACE("Destroying CountTo10Domain."); };

        void update();

    private:
        i32 counter_;
    };
} // namespace v
