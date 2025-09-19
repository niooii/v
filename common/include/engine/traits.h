//
// Created by niooi on 9/19/2025.
//

#pragma once

#include <type_traits>

namespace v {

    /// Inherit from this type to specify what type a type should be coerced to when querying the engine's registry via engine::view<Type...>(). 
    ///
    /// For types that are stored in a registry as a pointer, for example,
    /// this trait allows users to use the engine::view<> method to query by a 
    /// simple typename instead of the type pointer by inheriting from QueryBy<Type*>.
    template <typename QueryType>
    struct QueryBy {
        using query_type = QueryType;
    };

    template <typename T>
    concept InheritsFromQueryBy = requires { typename T::query_type; };

    template <typename T, bool HasQueryType = InheritsFromQueryBy<T>>
    struct query_type_extractor {
        using type = T;
    };

    template <typename T>
    struct query_type_extractor<T, true> {
        using type = typename T::query_type;
    };

    /// Type trait to transform types that inherit from QueryBy
    template <typename T>
    struct query_transform {
        using type = typename query_type_extractor<T>::type;
    };

    template <typename T>
    using query_transform_t = typename query_transform<T>::type;

} // namespace v
