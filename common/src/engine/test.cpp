//
// Created by niooi on 7/31/2025.
//

#include <engine/engine.h>
#include <engine/test.h>

namespace v {
    CountTo10Domain::CountTo10Domain(Engine& engine, const std::string& name) :
        Domain<CountTo10Domain>(engine, name), counter_(1)
    {}

    void CountTo10Domain::update()
    {
        if (counter_ <= 10)
        {
            // LOG_INFO("{}", counter_);
            counter_++;
        }

        if (counter_ > 10)
        {
            const entt::entity id = entity_;
            engine_.post_tick(
                [this, id]()
                {
                    if (engine_.registry().valid(id))
                        engine_.registry().destroy(id);
                });
        }
    }
} // namespace v
