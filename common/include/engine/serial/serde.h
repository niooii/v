//
// Created by niooi on 9/15/2025.
//

#include <ser20/archives/binary.hpp>
#include <ser20/ser20.hpp>
#include <ser20/types/string.hpp>
#include <sstream>

// provide convenience macros for ser20

/// Default ser20 binary implementation to serialize fields
#define SERIALIZE_FIELDS(...)   \
    template <class Archive>    \
    void serialize(Archive& ar) \
    {                           \
        ar(__VA_ARGS__);        \
    }

/// Default ser20 binary implementation of the ::parse and ::serialize functions.
#define SERDE_IMPL(Name)                                                               \
    static Name parse(const u8* bytes, u64 len)                                        \
    {                                                                                  \
        std::istringstream stream(                                                     \
            std::string(reinterpret_cast<const char*>(bytes), len), std::ios::binary); \
        ser20::BinaryInputArchive archive(stream);                                     \
        Name                      obj;                                                 \
        archive(obj);                                                                  \
        return obj;                                                                    \
    }                                                                                  \
    std::vector<std::byte> serialize() const                                           \
    {                                                                                  \
        std::ostringstream os(std::ios::binary);                                       \
        {                                                                              \
            ser20::BinaryOutputArchive oarchive(os);                                   \
            oarchive(*this);                                                           \
        }                                                                              \
                                                                                       \
        std::string str = os.str();                                                    \
                                                                                       \
        /* TODO! can i not memcpy this jus reinterpret or something */                 \
        std::vector<std::byte> bytes(str.size());                                      \
        std::memcpy(bytes.data(), str.data(), str.size());                             \
                                                                                       \
        return bytes;                                                                  \
    }
