#ifndef HASH_HPP
#define HASH_HPP

#include <cstddef>
#include <optional>
#include <array>
#include "../bundled/MurmurHash3.hpp"

namespace hash {

class double_hash64
{
    public:
        typedef uint64_t hash_type;

        static std::array<uint64_t, 2> hash(uint8_t const* key, uint32_t keylen, uint32_t seed) noexcept
        {
            std::array<uint64_t, 2> hval;
            MurmurHash3_x64_128(reinterpret_cast<const void*>(key), keylen, seed, hval.data());
            return hval;
        }

        template <typename T>
        static std::array<uint64_t, 2> hash(T val, uint64_t seed) noexcept
        {
            return hash(reinterpret_cast<uint8_t*>(&val), sizeof(T), seed);
        }

        std::array<uint64_t, 2> operator()(uint8_t const* key, uint32_t keylen, uint32_t seed) const noexcept
        {
            // std::array<uint64_t, 2> hval;
            // MurmurHash3_x64_128(reinterpret_cast<const void*>(key), keylen, seed, hval.data());
            // return hval;
            return hash(key, keylen, seed);
        }

        template <typename T>
        std::array<uint64_t, 2> operator()(T val, uint64_t seed) const noexcept
        {
            // return operator()(reinterpret_cast<uint8_t*>(&val), sizeof(T), seed);
            return hash(val, seed);
        }
};

class hash64
{
    public:
        typedef uint64_t hash_type;

        static uint64_t hash(uint8_t const* key, uint32_t keylen, uint32_t seed) noexcept
        {
            return double_hash64::hash(key, keylen, seed)[0];
        }

        template <typename T>
        static uint64_t hash(T val, uint64_t seed) noexcept
        {
            return hash(reinterpret_cast<uint8_t*>(&val), sizeof(T), seed);
        }

        uint64_t operator()(uint8_t const* key, uint32_t keylen, uint32_t seed) const noexcept
        {
            return hash(key, keylen, seed);
        }

        template <typename T>
        uint64_t operator()(T val, uint64_t seed) const noexcept
        {
            return hash(reinterpret_cast<uint8_t*>(&val), sizeof(T), seed);
        }
};

}

#endif // HASH_HPP