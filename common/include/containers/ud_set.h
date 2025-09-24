#pragma once

#include <functional>
#include <unordered_dense.h>

namespace v {
    template <
        class Key, class Hash = ankerl::unordered_dense::hash<Key>,
        class KeyEqual = std::equal_to<Key>>
    using ud_set = ankerl::unordered_dense::set<Key, Hash, KeyEqual>;
}
