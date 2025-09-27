//
// Created by niooi on 9/26/2025.
//

#pragma once

#include <prelude.h>
#include "domain/context.h"

namespace v {
    class Client : public Context {
        Client(Engine& engine);
    };
}
