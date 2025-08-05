//
// Created by niooi on 7/31/2025.
//

#include <domain/test.h>
#include <engine/engine.h>

namespace v {
    CountTo10Domain::CountTo10Domain(Engine& engine, const std::string& name) :
        Domain(engine, name), counter_(1)
    {
    }
    
    void CountTo10Domain::update()
    {
        if (counter_ <= 10) {
            // LOG_INFO("{}", counter_);
            counter_++;
        }
        
        if (counter_ > 10) {
            engine_.queue_destroy_domain(entity_);
        }
    }
} // namespace v