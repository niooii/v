//
// Created by niooi on 9/15/2025.
//

#pragma once

#include <array>

#include <rfl.hpp>

#include <glm/gtc/quaternion.hpp>
#include <glm/mat2x2.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

// implementation of 3rd party useful types that may be serialized often
namespace rfl {
    // vec2
    template <typename T, glm::qualifier Q>
    struct Reflector<glm::vec<2, T, Q>> {
        using ReflType = std::array<T, 2>;
        static ReflType to(const glm::vec<2, T, Q>& v) noexcept { return { v.x, v.y }; }
        static glm::vec<2, T, Q> from(const ReflType& r)
        {
            return glm::vec<2, T, Q>{ r[0], r[1] };
        }
    };

    // vec3
    template <typename T, glm::qualifier Q>
    struct Reflector<glm::vec<3, T, Q>> {
        using ReflType = std::array<T, 3>;
        static ReflType to(const glm::vec<3, T, Q>& v) noexcept
        {
            return { v.x, v.y, v.z };
        }
        static glm::vec<3, T, Q> from(const ReflType& r)
        {
            return glm::vec<3, T, Q>{ r[0], r[1], r[2] };
        }
    };

    // vec4
    template <typename T, glm::qualifier Q>
    struct Reflector<glm::vec<4, T, Q>> {
        using ReflType = std::array<T, 4>;
        static ReflType to(const glm::vec<4, T, Q>& v) noexcept
        {
            return { v.x, v.y, v.z, v.w };
        }
        static glm::vec<4, T, Q> from(const ReflType& r)
        {
            return glm::vec<4, T, Q>{ r[0], r[1], r[2], r[3] };
        }
    };

    // mat2x2
    template <typename T, glm::qualifier Q>
    struct Reflector<glm::mat<2, 2, T, Q>> {
        using ReflType = std::array<T, 4>;
        static ReflType to(const glm::mat<2, 2, T, Q>& m) noexcept
        {
            return { m[0].x, m[0].y, m[1].x, m[1].y };
        }
        static glm::mat<2, 2, T, Q> from(const ReflType& r)
        {
            return glm::mat<2, 2, T, Q>{ glm::vec<2, T, Q>{ r[0], r[1] },
                                         glm::vec<2, T, Q>{ r[2], r[3] } };
        }
    };

    // mat3x3
    template <typename T, glm::qualifier Q>
    struct Reflector<glm::mat<3, 3, T, Q>> {
        using ReflType = std::array<T, 9>;
        static ReflType to(const glm::mat<3, 3, T, Q>& m) noexcept
        {
            return { m[0].x, m[0].y, m[0].z, m[1].x, m[1].y,
                     m[1].z, m[2].x, m[2].y, m[2].z };
        }
        static glm::mat<3, 3, T, Q> from(const ReflType& r)
        {
            return glm::mat<3, 3, T, Q>{ glm::vec<3, T, Q>{ r[0], r[1], r[2] },
                                         glm::vec<3, T, Q>{ r[3], r[4], r[5] },
                                         glm::vec<3, T, Q>{ r[6], r[7], r[8] } };
        }
    };

    // mat4x4
    template <typename T, glm::qualifier Q>
    struct Reflector<glm::mat<4, 4, T, Q>> {
        using ReflType = std::array<T, 16>;
        static ReflType to(const glm::mat<4, 4, T, Q>& m) noexcept
        {
            return { m[0].x, m[0].y, m[0].z, m[0].w, m[1].x, m[1].y, m[1].z, m[1].w,
                     m[2].x, m[2].y, m[2].z, m[2].w, m[3].x, m[3].y, m[3].z, m[3].w };
        }
        static glm::mat<4, 4, T, Q> from(const ReflType& r)
        {
            return glm::mat<4, 4, T, Q>{ glm::vec<4, T, Q>{ r[0], r[1], r[2], r[3] },
                                         glm::vec<4, T, Q>{ r[4], r[5], r[6], r[7] },
                                         glm::vec<4, T, Q>{ r[8], r[9], r[10], r[11] },
                                         glm::vec<4, T, Q>{ r[12], r[13], r[14],
                                                            r[15] } };
        }
    };

    // quaternion
    template <typename T, glm::qualifier Q>
    struct Reflector<glm::qua<T, Q>> {
        using ReflType = std::array<T, 4>;
        static ReflType to(const glm::qua<T, Q>& q) noexcept
        {
            return { q.w, q.x, q.y, q.z };
        }
        static glm::qua<T, Q> from(const ReflType& r)
        {
            return glm::qua<T, Q>{ r[0], r[1], r[2], r[3] };
        }
    };
} // namespace rfl

// NOW include CBOR after Reflector specializations are defined
#include <rfl/cbor.hpp>

// Serialization macros using reflect-cpp with CBOR binary format

/// Skip a field during serialization (field gets default-constructed on deserialization).
/// Usage: SERDE_SKIP(std::mutex) my_lock;
#define SERDE_SKIP(Type) rfl::Skip<Type>

/// Generates parse() and serialize() methods using reflect-cpp CBOR binary format.
/// All struct fields are automatically serialized unless wrapped with SERDE_SKIP().
///
/// Deserialization behavior:
/// - Fields wrapped with SERDE_SKIP() are default-constructed on the receiver side
/// - All other fields are populated from the binary data
#define SERDE_IMPL(Name)                                                                 \
    static Name parse(const u8* bytes, u64 len)                                          \
    {                                                                                    \
        auto result = rfl::cbor::read<Name>(                                             \
            std::span<const std::byte>(reinterpret_cast<const std::byte*>(bytes), len)); \
        if (!result)                                                                     \
        {                                                                                \
            throw std::runtime_error(                                                    \
                "Failed to deserialize " #Name ": " + result.error().what());            \
        }                                                                                \
        return result.value();                                                           \
    }                                                                                    \
    std::vector<std::byte> serialize() const                                             \
    {                                                                                    \
        auto                   char_bytes = rfl::cbor::write(*this);                     \
        std::vector<std::byte> bytes(char_bytes.size());                                 \
        std::memcpy(bytes.data(), char_bytes.data(), char_bytes.size());                 \
        return bytes;                                                                    \
    }
