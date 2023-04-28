#ifndef BIT_OPERATIONS_HPP
#define BIT_OPERATIONS_HPP

#include <cstdint>
#include <cstddef>
#include <array>

namespace bit {

typedef
#if defined(UINT64_MAX)
uint64_t 
#elif defined(UINT32_MAX)
uint32_t
#elif defined(UINT16_MAX)
uint16_t
#else
uint8_t
#endif
max_width_native_type;

static const std::array<uint8_t, 256> popvalues = {
    0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
    4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
};

#if __cplusplus >= 202002L
template <typename T>
constexpr int std_popcount(T x) {return std::popcount(x);}
#elif __SSE4_2__
#if defined(__GNUC__) || defined(__GNUG__)
#pragma GCC target("sse4")
#endif
constexpr int sse4_popcount(uint8_t x) {return _mm_popcnt_u8(x);}
constexpr int sse4_popcount(uint16_t x) {return _mm_popcnt_u16(x);}
constexpr int sse4_popcount(uint32_t x) {return _mm_popcnt_u32(x);}
constexpr int sse4_popcount(uint64_t x) {return _mm_popcnt_u64(x);}
#elif defined(__clang__) // use same extension as gcc
constexpr int clang_popcount(uint8_t x) {return __builtin_popcount(x);}
constexpr int clang_popcount(uint16_t x) {return __builtin_popcount(x);}
constexpr int clang_popcount(uint32_t x) {return __builtin_popcount(x);}
constexpr int clang_popcount(uint64_t x) {return __builtin_popcountll(x);}
#elif defined(__GNUC__) || defined(__GNUG__)
constexpr int gcc_popcount(uint8_t x) {return __builtin_popcount(x);}
constexpr int gcc_popcount(uint16_t x) {return __builtin_popcount(x);}
constexpr int gcc_popcount(uint32_t x) {return __builtin_popcount(x);}
constexpr int gcc_popcount(uint64_t x) {return __builtin_popcountll(x);}
#elif defined(_MSC_VER)
#include <intrin.h>
constexpr int mvsc_popcount(uint8_t x) {return __popcnt16(x);}
constexpr int mvsc_popcount(uint16_t x) {return __popcnt16(x);}
constexpr int mvsc_popcount(uint32_t x) {return __popcnt(x);}
constexpr int mvsc_popcount(uint64_t x) {return __popcnt64(x);}
#else
template <typename T>
constexpr int emulated_popcount(T x) 
{
    x = x - ((x >> 1) & (T)~(T)0/3);
    x = (x & (T)~(T)0/15*3) + ((x >> 2) & (T)~(T)0/15*3);
    x = (x + (x >> 4)) & (T)~(T)0/255*15;
    return (T)(x * ((T)~(T)0/255)) >> (sizeof(T) - 1) * 8;
}
#endif

template <typename T>
constexpr int popcount(T x) noexcept
{
    #if __cplusplus >= 202002L
        return std_popcount(x);
    #elif __SSE4_2__
        return sse4_popcount(x);
    #elif defined(__clang__) // use same extension as gcc
        return clang_popcount(x);
    #elif defined(__GNUC__) || defined(__GNUG__)
        return gcc_popcount(x);
    #elif defined(_MSC_VER)
        return mvsc_popcount(x);
    #else
        return emulated_popcount(x);
    #endif
}

template <class Vector>
inline std::size_t popcount(Vector vec, std::size_t idx)
{
    std::size_t popc = 0;
    for (std::size_t i = 0; i < idx; ++i) {
        popc += popcount(vec.at(i));
    }
    return popc;
}

inline std::size_t rank(uint8_t const * const arr, std::size_t bit_idx) 
{
    typedef max_width_native_type view_t;
    view_t const * const p = reinterpret_cast<view_t const * const>(arr);
    const std::size_t view_bit_size = sizeof(view_t) * 8;
    const std::size_t block_idx = bit_idx / view_bit_size;
    // std::cerr << "bit idx = " << bit_idx << ", block idx = " << block_idx << std::endl;

    std::size_t popc = 0;
    for (std::size_t i = 0; i < block_idx; ++i) popc += popcount(p[i]);

    // std::cerr << "partial sum = " << popc << "\n";

    view_t buffer = static_cast<view_t>(0);
    const std::size_t rem_bit_idx = (bit_idx % view_bit_size);
    const std::size_t shift = (rem_bit_idx + 1) % 8;
    const std::size_t rem_bytes = rem_bit_idx / 8 + 1;
    assert(shift < 8);
    // std::cerr << "block idx = " << block_idx << ", rem bit idx = " << rem_bit_idx << ", rem bytes = " << rem_bytes << ", shift = " << shift << std::endl;
    memcpy(reinterpret_cast<void*>(&buffer), reinterpret_cast<void const *>(&arr[block_idx * sizeof(view_t)]), rem_bytes);
    
    auto buffer_view = reinterpret_cast<uint8_t*>(&buffer);
    buffer_view[rem_bytes-1] &= ~((uint8_t(1) << shift) - uint8_t(1));
    popc += popcount(buffer);
    return popc;
}

template <class Vector>
inline std::size_t rank(Vector vec, std::size_t stop, std::size_t start = 0)
{
    if (start > vec.size() or stop > vec.size()) throw std::out_of_range("[rank] indexes must be smaller than vector size");
    if (start > stop) throw std::logic_error("[rank] stop index < start");
    const std::size_t byte_start = start * sizeof(typename Vector::value_type);
    const std::size_t byte_stop = stop * sizeof(typename Vector::value_type);
    return rank(reinterpret_cast<uint8_t const *>(&vec.data()[byte_start]), byte_stop);
}

} // namespace bit

#endif // BIT_OPERATIONS_HPP