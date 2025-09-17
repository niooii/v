//
// Created by niooi on 9/15/2025.
//

#include <ser20/ser20.hpp>
#include <ser20/archives/binary.hpp>
#include <sstream>

// provide convenience macros for ser20

#define SERIALIZE_FIELDS(...)   \
    template <class Archive>    \
    void serialize(Archive& ar) \
    {                           \
        ar(__VA_ARGS__);        \
    }

#define SER20_PARSE_FN(Name) \
    static Name parse(const u8* bytes, u64 len) { \
        std::istringstream stream(std::string(reinterpret_cast<const char*>(bytes), len), std::ios::binary); \
        ser20::BinaryInputArchive archive(stream); \
        Name obj; \
        archive(obj); \
        return obj; \
    }
