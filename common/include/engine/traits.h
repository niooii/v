//
// Created by niooi on 9/19/2025.
//

#pragma once

#include <type_traits>

namespace v {

    /// Inherit from this type to specify what type a type should be coerced to when
    /// querying the engine's registry via engine::view<Type...>().
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

    /// Concept to detect if a type has a .get() method that returns a pointer
    template <typename T>
    concept HasGetMethod = requires(T& t) {
        { t.get() };
    };

    /// Utility to convert a pointer to a type's storage type (from query_transform_t)
    /// to a raw pointer.
    /// Automatically handles owned_ptr, unique_ptr, raw pointers, etc.
    template <typename T>
    auto to_return_ptr(query_transform_t<T>* storage_ptr)
    {
        if constexpr (InheritsFromQueryBy<T>)
        {
            if constexpr (HasGetMethod<query_transform_t<T>>)
            {
                // is a smart pointer or a type that implements .get()
                // TODO! should check if get actually returns a raw pointer?
                return storage_ptr ? storage_ptr->get() : nullptr;
            }
            else if constexpr (std::is_pointer_v<query_transform_t<T>>)
            {
                // dereference a double pointer
                return storage_ptr ? *storage_ptr : nullptr;
            }
            else
            {
                static_assert(
                    false,
                    "QueryBy type must either have a .get() method that returns a raw "
                    "pointer T*, or be a raw pointer");
            }
        }
        else
        {
            return storage_ptr;
        }
    }

    /// Const overload
    template <typename T>
    auto to_return_ptr(const query_transform_t<T>* storage_ptr)
    {
        if constexpr (InheritsFromQueryBy<T>)
        {
            if constexpr (HasGetMethod<query_transform_t<T>>)
            {
                // is a smart pointer or a type that implements .get()
                // TODO! should check if get actually returns a raw pointer?
                return storage_ptr ? storage_ptr->get() : nullptr;
            }
            else if constexpr (std::is_pointer_v<query_transform_t<T>>)
            {
                // dereference a double pointer
                return storage_ptr ? *storage_ptr : nullptr;
            }
            else
            {
                static_assert(
                    false,
                    "QueryBy type must either have a .get() method that returns a raw "
                    "pointer T*, or be a raw pointer");
            }
        }
        else
        {
            return storage_ptr;
        }
    }

} // namespace v
