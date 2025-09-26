//
// Created by niooi on 7/28/2025.
//

#include <domain/domain.h>
#include <engine/engine.h>

namespace v {
    DomainBase::DomainBase(Engine& engine, std::string name) :
        engine_(engine), name_(std::move(name))
    {
        entity_ = engine_.registry().create();
    }

    DomainBase::~DomainBase()
    {
        // Entity lifetime is managed externally; do not access other domains/contexts
        // here.
    }
} // namespace v
