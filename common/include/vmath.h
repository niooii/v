//
// Created by niooi on 10/6/2025.
//

#pragma once

#include <defs.h>
#include <glm/common.hpp>
#include <glm/detail/qualifier.hpp>
#include <glm/exponential.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/vec1.hpp> // for scalars

namespace v {
    namespace math_detail {
        /// type traits to promote a scalar to a vector, and back to a scalar so i don't
        /// have to overload with scalar impls
        template <typename T>
        struct vec_traits {
            static constexpr bool          is_vec = false;
            static constexpr glm::length_t L      = 1; // scalars have 1 copmonent
            using scalar                          = T; // element is itself
            using vec = glm::vec<1, T, glm::defaultp>; // promoted type
            static constexpr glm::qualifier qual = glm::defaultp;
        };

        template <glm::length_t L, typename T, glm::qualifier Q>
        struct vec_traits<glm::vec<L, T, Q>> {
            static constexpr bool          is_vec = true;
            static constexpr glm::length_t Len    = L; // number of components
            using scalar                          = T; // underlying elem type
            using vec                             = glm::vec<L, T, Q>; // leave it alone
            static constexpr glm::qualifier qual  = Q;
        };

        // promotes a scalar into a vector, or leaves a vector untouched
        template <typename V>
        FORCEINLINE auto promote(const V& v) -> typename vec_traits<V>::vec
        {
            using traits = vec_traits<V>;
            if constexpr (traits::is_vec)
                return v; // already a vector
            else
            {
                using T = typename traits::scalar;
                return typename traits::vec(static_cast<T>(v)); // scalar -> vec1(T(v))
            }
        }

        // demotes a vec1 into a scalar, or leaves a vector untouched
        template <typename Orig, typename VecLike>
        FORCEINLINE Orig demote(const VecLike& v)
        {
            using traits = vec_traits<Orig>;
            if constexpr (traits::is_vec)
                return v;
            else
            {
                using T = typename traits::scalar;
                return static_cast<Orig>(static_cast<T>(v[0])); // vec1 -> scalar
            }
        }
    } // namespace math_detail

    /// Clamps all components of the vector to [0, 1].
    template <typename V>
    FORCEINLINE V saturate(const V& v)
    {
        using namespace math_detail;
        using traits   = vec_traits<V>;
        using scalar_t = typename traits::scalar;

        auto promoted = promote(v);
        auto result   = glm::clamp(promoted, scalar_t(0), scalar_t(1));
        return demote<V>(result);
    }

    /// Clamps value/vector between min and max (scalar bounds).
    template <typename V>
    FORCEINLINE V clamp(
        const V& v, typename math_detail::vec_traits<V>::scalar lo,
        typename math_detail::vec_traits<V>::scalar hi)
    {
        using namespace math_detail;
        auto promoted = promote(v);
        auto result   = glm::clamp(promoted, lo, hi);
        return demote<V>(result);
    }

    /// Clamps value/vector between min and max (vector bounds).
    template <typename V>
    FORCEINLINE V clamp(const V& v, const V& lo, const V& hi)
    {
        using namespace math_detail;
        auto promoted_v  = promote(v);
        auto promoted_lo = promote(lo);
        auto promoted_hi = promote(hi);
        auto result      = glm::clamp(promoted_v, promoted_lo, promoted_hi);
        return demote<V>(result);
    }

    /// Raises value/vector to scalar power.
    template <typename V>
    FORCEINLINE V pow(const V& base, typename math_detail::vec_traits<V>::scalar exponent)
    {
        using namespace math_detail;
        using traits     = vec_traits<V>;
        using promoted_t = typename traits::vec;

        auto promoted_base = promote(base);
        auto promoted_exp  = promoted_t(exponent); // broadcast scalar to all components
        auto result        = glm::pow(promoted_base, promoted_exp);
        return demote<V>(result);
    }

    /// Raises value/vector to vector power (component-wise).
    template <typename V>
    FORCEINLINE V pow(const V& base, const V& exponent)
    {
        using namespace math_detail;
        auto promoted_base = promote(base);
        auto promoted_exp  = promote(exponent);
        auto result        = glm::pow(promoted_base, promoted_exp);
        return demote<V>(result);
    }

    /// Applies ceil() to each component.
    template <
        typename V,
        typename = std::enable_if_t<
            std::is_floating_point_v<typename math_detail::vec_traits<V>::scalar>>>
    FORCEINLINE V ceil(const V& v)
    {
        using namespace math_detail;
        auto promoted = promote(v);
        auto result   = glm::ceil(promoted);
        return demote<V>(result);
    }

    /// Applies floor() to each component.
    template <
        typename V,
        typename = std::enable_if_t<
            std::is_floating_point_v<typename math_detail::vec_traits<V>::scalar>>>
    FORCEINLINE V floor(const V& v)
    {
        using namespace math_detail;
        auto promoted = promote(v);
        auto result   = glm::floor(promoted);
        return demote<V>(result);
    }

    /// Returns the largest component of the vector.
    template <typename V>
    FORCEINLINE typename math_detail::vec_traits<V>::scalar max_component(const V& v)
    {
        using namespace math_detail;
        using traits     = vec_traits<V>;
        using promoted_t = typename traits::vec;
        using scalar_t   = typename traits::scalar;

        auto promoted = promote(v);

        scalar_t                m   = promoted[0];
        constexpr glm::length_t len = promoted_t::length();
        for (glm::length_t i = 1; i < len; ++i)
            m = std::max(m, promoted[i]);
        return m;
    }

    /// Returns the smallest component of the vector.
    template <typename V>
    FORCEINLINE typename math_detail::vec_traits<V>::scalar min_component(const V& v)
    {
        using namespace math_detail;
        using traits     = vec_traits<V>;
        using promoted_t = typename traits::vec;
        using scalar_t   = typename traits::scalar;

        auto promoted = promote(v);

        scalar_t                m   = promoted[0];
        constexpr glm::length_t len = promoted_t::length();
        for (glm::length_t i = 1; i < len; ++i)
            m = std::min(m, promoted[i]);
        return m;
    }


    template <
        typename V,
        typename = std::enable_if_t<
            std::is_integral_v<typename math_detail::vec_traits<V>::scalar>>>
    FORCEINLINE V ipow(const V& v, u32 e)
    {
        using namespace math_detail;
        using traits     = vec_traits<V>;
        using promoted_t = typename traits::vec;

        promoted_t              base = promote(v);
        promoted_t              res(1);
        u32                     n   = e;
        constexpr glm::length_t len = promoted_t::length();
        while (n)
        {
            if (n & 1u)
                for (glm::length_t i = 0; i < len; ++i)
                    res[i] *= base[i];
            n >>= 1u;
            if (n)
                for (glm::length_t i = 0; i < len; ++i)
                    base[i] *= base[i];
        }
        return demote<V>(res);
    }

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

    // Specialization for fast power of 2 stuff
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
