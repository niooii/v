//
// Created by niooi on 10/6/2025.
//

#pragma once

#include <defs.h>
#include <glm/common.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/vec1.hpp> // for scalars

namespace v {
    // ------------------------------
    // Component-wise helpers
    // ------------------------------

    /// Clamps all components of the vector to [lo, hi].
    /// Scalars for lo/hi are broadcasted.
    template <
        typename T, glm::length_t L, glm::qualifier Q, typename S,
        typename = std::enable_if_t<std::is_arithmetic_v<S>>>
    FORCEINLINE glm::vec<L, T, Q> clamp(const glm::vec<L, T, Q>& v, S lo, S hi)
    {
        return glm::clamp(v, glm::vec<L, T, Q>(T(lo)), glm::vec<L, T, Q>(T(hi)));
    }

    /// Clamps all components of the vector to [lo, hi] component-wise.
    template <typename T, glm::length_t L, glm::qualifier Q>
    FORCEINLINE glm::vec<L, T, Q> clamp(
        const glm::vec<L, T, Q>& v, const glm::vec<L, T, Q>& lo,
        const glm::vec<L, T, Q>& hi)
    {
        return glm::clamp(v, lo, hi);
    }

    /// Clamps all components of the vector to [0, 1].
    template <typename T, glm::length_t L, glm::qualifier Q>
    FORCEINLINE glm::vec<L, T, Q> saturate(const glm::vec<L, T, Q>& v)
    {
        return glm::clamp(v, T(0), T(1));
    }

    /// Applies ceil() to each component.
    template <
        typename T, glm::length_t L, glm::qualifier Q,
        typename = std::enable_if_t<std::is_floating_point_v<T>>>
    FORCEINLINE glm::vec<L, T, Q> ceil(const glm::vec<L, T, Q>& v)
    {
        return glm::ceil(v);
    }

    /// Applies floor() to each component.
    template <
        typename T, glm::length_t L, glm::qualifier Q,
        typename = std::enable_if_t<std::is_floating_point_v<T>>>
    FORCEINLINE glm::vec<L, T, Q> floor(const glm::vec<L, T, Q>& v)
    {
        return glm::floor(v);
    }

    // ------------------------------
    // Reductions
    // ------------------------------

    /// Returns the largest component of the vector.
    template <typename T, glm::length_t L, glm::qualifier Q>
    FORCEINLINE T max_component(const glm::vec<L, T, Q>& v)
    {
        T m = v[0];
        for (glm::length_t i = 1; i < L; ++i)
            m = std::max(m, v[i]);
        return m;
    }

    /// Returns the smallest component of the vector.
    template <typename T, glm::length_t L, glm::qualifier Q>
    FORCEINLINE T min_component(const glm::vec<L, T, Q>& v)
    {
        T m = v[0];
        for (glm::length_t i = 1; i < L; ++i)
            m = std::min(m, v[i]);
        return m;
    }

    // ------------------------------
    // Powers
    // ------------------------------

    /// Raises each component to the same exponent e.
    /// Valid for floating-point vectors only.
    template <
        typename T, glm::length_t L, glm::qualifier Q, typename S,
        typename =
            std::enable_if_t<std::is_floating_point_v<T> && std::is_arithmetic_v<S>>>
    FORCEINLINE glm::vec<L, T, Q> pow(const glm::vec<L, T, Q>& v, S e)
    {
        return glm::pow(v, T(e));
    }

    /// Raises each component to its corresponding per-component exponent.
    /// Valid for floating-point vectors only.
    template <
        typename T, glm::length_t L, glm::qualifier Q,
        typename = std::enable_if_t<std::is_floating_point_v<T>>>
    FORCEINLINE glm::vec<L, T, Q>
                pow(const glm::vec<L, T, Q>& v, const glm::vec<L, T, Q>& e)
    {
        return glm::pow(v, e);
    }

    /// Integer power (non-negative exponent) for integer vectors.
    /// Uses exponentiation by squaring per component.
    template <
        typename Int, glm::length_t L, glm::qualifier Q,
        typename = std::enable_if_t<std::is_integral_v<Int> && std::is_signed_v<Int>>>
    FORCEINLINE glm::vec<L, Int, Q> ipow(const glm::vec<L, Int, Q>& v, u32 e)
    {
        glm::vec<L, Int, Q> base = v;
        glm::vec<L, Int, Q> res(1);
        u32                 n = e;
        while (n)
        {
            if (n & 1u)
                for (glm::length_t i = 0; i < L; ++i)
                    res[i] *= base[i];
            n >>= 1u;
            if (n)
                for (glm::length_t i = 0; i < L; ++i)
                    base[i] *= base[i];
        }
        return res;
    }

    /// Integer power (non-negative exponent) for unsigned integer vectors.
    /// Uses exponentiation by squaring per component.
    template <
        typename UInt, glm::length_t L, glm::qualifier Q,
        typename = std::enable_if_t<std::is_unsigned_v<UInt>>>
    FORCEINLINE glm::vec<L, UInt, Q> ipow(const glm::vec<L, UInt, Q>& v, u32 e)
    {
        glm::vec<L, UInt, Q> base = v;
        glm::vec<L, UInt, Q> res(1);
        u32                  n = e;
        while (n)
        {
            if (n & 1u)
                for (glm::length_t i = 0; i < L; ++i)
                    res[i] *= base[i];
            n >>= 1u;
            if (n)
                for (glm::length_t i = 0; i < L; ++i)
                    base[i] *= base[i];
        }
        return res;
    }

    /// Integer power (non-negative exponent) for scalar integers.
    /// Matches semantics of the vector ipow.
    template <typename Int, typename = std::enable_if_t<std::is_integral_v<Int>>>
    FORCEINLINE Int ipow(Int base, u32 e)
    {
        Int res = 1;
        u32 n   = e;
        Int b   = base;
        while (n)
        {
            if (n & 1u)
                res *= b;
            n >>= 1u;
            if (n)
                b *= b;
        }
        return res;
    }

    // ------------------------------
    // Log base N (ceil/floor) — floating versions
    // ------------------------------

    /// Returns floor(log_base(x)).
    /// Returns INT_MIN if x <= 0 or base <= 1.
    FORCEINLINE i32 floor_log(f64 x, f64 base)
    {
        if (!(x > 0.0) || !(base > 1.0))
            return std::numeric_limits<i32>::min();
        return static_cast<i32>(std::floor(std::log(x) / std::log(base)));
    }

    /// Returns ceil(log_base(x)).
    /// Returns INT_MIN if x <= 0 or base <= 1.
    FORCEINLINE i32 ceil_log(f64 x, f64 base)
    {
        if (!(x > 0.0) || !(base > 1.0))
            return std::numeric_limits<i32>::min();
        return static_cast<i32>(std::ceil(std::log(x) / std::log(base)));
    }

    // ------------------------------
    // Log base N (ceil/floor) — integer-safe versions
    // ------------------------------

    /// Returns floor(log_base(x)) for unsigned integers.
    /// Returns INT_MIN if base < 2 or x == 0.
    template <typename UInt>
    FORCEINLINE i32 ifloor_log(UInt x, UInt base)
    {
        STATIC_ASSERT(std::is_unsigned_v<UInt>, "ifloor_log expects an unsigned type");
        if (base < 2 || x == 0)
            return std::numeric_limits<i32>::min();
        i32 p = 0;
        while (x >= base)
        {
            x /= base;
            ++p;
        }
        return p;
    }

    /// Returns ceil(log_base(x)) for unsigned integers.
    /// Returns INT_MIN if base < 2 or x == 0.
    template <typename UInt>
    FORCEINLINE i32 iceil_log(UInt x, UInt base)
    {
        STATIC_ASSERT(std::is_unsigned_v<UInt>, "iceil_log expects an unsigned type");
        if (base < 2 || x == 0)
            return std::numeric_limits<i32>::min();
        UInt v = 1;
        i32  p = 0;
        while (v < x)
        {
            if (v > std::numeric_limits<UInt>::max() / base)
            {
                ++p;
                break;
            }
            v *= base;
            ++p;
        }
        return p;
    }

    // ------------------------------
    // Fast specializations for power-of-two bases
    // ------------------------------

    namespace detail {

        /// Returns floor(log2(x)) using CLZ macros.
        /// Returns INT_MIN if x == 0.
        template <typename UInt>
        FORCEINLINE i32 ilog2(UInt x)
        {
            STATIC_ASSERT(std::is_unsigned_v<UInt>, "ilog2 expects an unsigned type");
            if (x == 0)
                return std::numeric_limits<i32>::min();

            if constexpr (sizeof(UInt) == 8)
            {
                return 63 - static_cast<i32>(CLZ64(x));
            }
            else if constexpr (sizeof(UInt) == 4)
            {
                return 31 - static_cast<i32>(CLZ(x));
            }
            else if constexpr (sizeof(UInt) == 2)
            {
                u32 w = static_cast<u32>(x);
                return 31 - static_cast<i32>(CLZ(w)) - 16;
            }
            else
            {
                i32 p = -1;
                while (x)
                {
                    x >>= 1;
                    ++p;
                }
                return p;
            }
        }

    } // namespace detail

    /// Returns floor(log_{2^k}(x)).
    /// Equivalent to floor(log2(x) / k).
    /// Returns INT_MIN if k == 0 or x == 0.
    template <typename UInt>
    FORCEINLINE i32 floor_log_pow2(UInt x, u32 k)
    {
        STATIC_ASSERT(
            std::is_unsigned_v<UInt>, "floor_log_pow2 expects an unsigned type");
        if (k == 0 || x == 0)
            return std::numeric_limits<i32>::min();
        const i32 lg2 = detail::ilog2(x);
        if (lg2 < 0)
            return std::numeric_limits<i32>::min();
        return lg2 / static_cast<i32>(k);
    }

    /// Returns ceil(log_{2^k}(x)).
    /// Equivalent to ceil(log2(x) / k).
    /// Returns INT_MIN if k == 0 or x == 0.
    template <typename UInt>
    FORCEINLINE i32 ceil_log_pow2(UInt x, u32 k)
    {
        STATIC_ASSERT(std::is_unsigned_v<UInt>, "ceil_log_pow2 expects an unsigned type");
        if (k == 0 || x == 0)
            return std::numeric_limits<i32>::min();

        const i32 f = floor_log_pow2(x, k);
        if (f < 0)
            return f;

        const u32 shift = k * static_cast<u32>(f);
        if (shift >= 8u * sizeof(UInt))
            return f + 1;
        const UInt base_pow = UInt(1) << shift;

        return (base_pow == x) ? f : f + 1;
    }
} // namespace v
