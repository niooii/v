
//
// Created by niooi on 7/28/2025.
//

#pragma once

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <spdlog/spdlog.h>

// TODO! for some reason windows macros conflict with daxa check this out later
#ifdef _WIN32
    #ifdef OPAQUE
        #undef OPAQUE
    #endif
#endif

#include <stddef.h>
#include <stdint.h>

#define LOG_TRACE    SPDLOG_TRACE
#define LOG_DEBUG    SPDLOG_DEBUG
#define LOG_INFO     SPDLOG_INFO
#define LOG_WARN     SPDLOG_WARN
#define LOG_ERROR    SPDLOG_ERROR
#define LOG_CRITICAL SPDLOG_CRITICAL


#define TODO()                           \
    LOG_CRITICAL("Not yet implemented"); \
    throw new std::runtime_error("Not yet implemented");


typedef uint8_t       u8;
typedef uint16_t      u16;
typedef uint32_t      u32;
typedef uint64_t      u64;
typedef size_t        isize;
typedef int8_t        i8;
typedef int16_t       i16;
typedef int32_t       i32;
typedef int64_t       i64;
typedef float         f32;
typedef double        f64;
typedef unsigned char byte;

/// Bunch of utility stuff

#ifdef _MSC_VER
// typeof is a keyword since C23

    #define TYPEOF      typeof
    #define FORCEINLINE __forceinline
    #define NOINLINE    __declspec(noinline)

    #define PACKED_STRUCT __pragma(pack(push, 1)) struct
    #define END_PACKED_STRUCT \
        ;                     \
        __pragma(pack(pop))

    #define STATIC_ASSERT static_assert

    #define CTZ(x)        _tzcnt_u32(x) // Count trailing zeros (32-bit)
    #define CTZ64(x)      _tzcnt_u64(x) // Count trailing zeros (64-bit)
    #define CLZ(x)        _lzcnt_u32(x) // Count leading zeros (32-bit)
    #define CLZ64(x)      _lzcnt_u64(x) // Count leading zeros (64-bit)
    #define POPCOUNT(x)   __popcnt(x) // Population count (32-bit)
    #define POPCOUNT64(x) __popcnt64(x) // Population count (64-bit)
    #define BYTESWAP16(x) _byteswap_ushort(x) // Byte swap (16-bit)
    #define BYTESWAP32(x) _byteswap_ulong(x) // Byte swap (32-bit)
    #define BYTESWAP64(x) _byteswap_uint64(x) // Byte swap (64-bit)

    #define LIKELY(x)   (x) // No direct equivalent in MSVC
    #define UNLIKELY(x) (x) // Same here

    #define ALIGNED(x)       __declspec(align(x))
    #define UNREACHABLE()    __assume(0)
    #define PREFETCH(x)      _mm_prefetch((const char*)(x), _MM_HINT_T0)
    #define MEMORY_BARRIER() _ReadWriteBarrier()

#else
// GCC, Clang, and other compilers

    #define TYPEOF      typeof
    #define FORCEINLINE __attribute__((always_inline)) inline
    #define NOINLINE    __attribute__((noinline))

    #define PACKED_STRUCT     struct __attribute__((packed))
    #define END_PACKED_STRUCT ;

    #define STATIC_ASSERT _Static_assert

    #define CTZ(x)        __builtin_ctz(x) // Count trailing zeros (32-bit)
    #define CTZ64(x)      __builtin_ctzll(x) // Count trailing zeros (64-bit)
    #define CLZ(x)        __builtin_clz(x) // Count leading zeros (32-bit)
    #define CLZ64(x)      __builtin_clzll(x) // Count leading zeros (64-bit)
    #define POPCOUNT(x)   __builtin_popcount(x) // Population count (32-bit)
    #define POPCOUNT64(x) __builtin_popcountll(x) // Population count (64-bit)
    #define BYTESWAP16(x) __builtin_bswap16(x) // Byte swap (16-bit) - fixed typo
    #define BYTESWAP32(x) __builtin_bswap32(x) // Byte swap (32-bit)
    #define BYTESWAP64(x) __builtin_bswap64(x) // Byte swap (64-bit)

    #define LIKELY(x)   __builtin_expect(!!(x), 1)
    #define UNLIKELY(x) __builtin_expect(!!(x), 0)

    #define ALIGNED(x)       __attribute__((aligned(x)))
    #define UNREACHABLE()    __builtin_unreachable()
    #define PREFETCH(x)      __builtin_prefetch(x)
    #define MEMORY_BARRIER() __asm__ volatile("" ::: "memory")

#endif

namespace v {
    /// Returns a unique typename for a given type.
    /// Using __PRETTY_FUNCTION__ which works with both GCC and Clang
    template <typename T>
    constexpr std::string_view type_name()
    {
        constexpr std::string_view func_name{ __PRETTY_FUNCTION__ };
        constexpr std::string_view prefix{ "[T = " };
        constexpr std::string_view suffix{ "]" };
        const auto                 start = func_name.find(prefix) + prefix.size();
        const auto                 end   = func_name.rfind(suffix);
        return func_name.substr(start, end - start);
    }

    // temp version for debugging
    template <typename T>
    void type_name_dbg()
    {
        constexpr std::string_view func_name{ __PRETTY_FUNCTION__ };
        LOG_DEBUG("type_name_dbg: {}", func_name);
    }
} // namespace v

STATIC_ASSERT(sizeof(u8) == 1, "expected u8 to be 1 byte.");
STATIC_ASSERT(sizeof(u16) == 2, "expected u16 to be 2 bytes.");
STATIC_ASSERT(sizeof(u32) == 4, "expected u32 to be 4 bytes.");
STATIC_ASSERT(sizeof(u64) == 8, "expected u64 to be 8 bytes.");
STATIC_ASSERT(sizeof(i8) == 1, "expected i8 to be 1 byte.");
STATIC_ASSERT(sizeof(i16) == 2, "expected i16 to be 2 bytes.");
STATIC_ASSERT(sizeof(i32) == 4, "expected i32 to be 4 bytes.");
STATIC_ASSERT(sizeof(i64) == 8, "expected i64 to be 8 bytes.");
STATIC_ASSERT(sizeof(f32) == 4, "expected f32 to be 4 bytes.");
STATIC_ASSERT(sizeof(f64) == 8, "expected f64 to be 8 bytes.");
