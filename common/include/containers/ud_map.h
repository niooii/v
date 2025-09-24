#pragma once

#include <functional>
#include <unordered_dense.h>

namespace v {
    // Keep signature minimal to let unordered_dense defaults kick in for allocator,
    // bucket type and internal bucket container.
    template <
        class Key, class T, class Hash = ankerl::unordered_dense::hash<Key>,
        class KeyEqual = std::equal_to<Key>>
    using ud_map = ankerl::unordered_dense::map<Key, T, Hash, KeyEqual>;
} // namespace v
