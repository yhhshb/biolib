#ifndef BIT_OPERATIONS_HPP
#define BIT_OPERATIONS_HPP

#include <stdexcept>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <array>
#include <vector>
#include <optional>
#include <cassert>

#if defined(__SSE4_2__)
// #if defined(__GNUC__) || defined(__GNUG__)
// #pragma GCC target("sse4") // redundant, since nmmintrin.h takes care of setting gcc flags
// #endif
extern "C" {
#include <nmmintrin.h>
}
#elif defined(_MSC_VER)
#include <intrin.h>
#endif

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

template <typename T>
inline std::size_t size() {return 8 * sizeof(T);}

template <typename T>
inline std::size_t size([[maybe_unused]] T x)
{
    return 8 * sizeof(T);
}

/*
    Good reference for built-in functions:
    http://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html
*/

/* position of the least significant bit (lsb) */
static inline std::size_t lsbll(unsigned long long x)
{
    assert(x > 0);              // if x is 0, the result is undefined
    return __builtin_ctzll(x);  // count trailing zeros (ctz)
}

/* position of the most significant bit (msb) */
template <typename T>
static inline std::size_t msbll(T x)
{
    return ::bit::size(x) - 1 - __builtin_clz(x);  // count leading zeros (clz)
}

inline std::optional<std::size_t> lsb(unsigned long long x)
{
    if (x) return static_cast<std::size_t>(__builtin_ctzll(x));
    return std::nullopt;
}

#if defined(__cplusplus) && (__cplusplus >= 202002L)
template <typename T>
constexpr int std_popcount(T x) {return std::popcount(x);}
#elif defined(__SSE4_2__)
inline int sse4_popcount(uint8_t x) {return _mm_popcnt_u32(x);}
inline int sse4_popcount(uint16_t x) {return _mm_popcnt_u32(x);}
inline int sse4_popcount(uint32_t x) {return _mm_popcnt_u32(x);}
inline int sse4_popcount(uint64_t x) {return _mm_popcnt_u64(x);}
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

/* TODO
inline int select(uint8_t x, std::size_t th)
{
    auto pc = popcount(x);
    assert(pc >= 0);
    assert(th < static_cast<std::size_t>(pc));
    // static_assert(false, "TODO");
    return 0;
}

inline int select(uint16_t x, std::size_t th)
{
    auto pc = popcount(x);
    assert(pc >= 0);
    assert(th < static_cast<std::size_t>(pc));
    // static_assert(false, "TODO");
    return 0;
}

inline int select(uint32_t x, std::size_t th)
{
    auto pc = popcount(x);
    assert(pc >= 0);
    assert(th < static_cast<std::size_t>(pc));
    static_assert(false, "TODO: Modify for 32 bit integers (write test suite beforehand)");
    unsigned int s;      // Output: Resulting position of bit with rank r [1-64]
    uint32_t a, b, c, d; // Intermediate temporaries for bit count.
    unsigned int t;      // Bit count temporary.

    // Do a normal parallel bit count for a 64-bit integer,
    // but store all intermediate steps.
    // a = (v & 0x5555...) + ((v >> 1) & 0x5555...);
    a =  v - ((v >> 1) & ~0UL/3);
    // b = (a & 0x3333...) + ((a >> 2) & 0x3333...);
    b = (a & ~0UL/5) + ((a >> 2) & ~0UL/5);
    // c = (b & 0x0f0f...) + ((b >> 4) & 0x0f0f...);
    c = (b + (b >> 4)) & ~0UL/0x11;
    // d = (c & 0x00ff...) + ((c >> 8) & 0x00ff...);
    d = (c + (c >> 8)) & ~0UL/0x101;
    t = (d >> 32) + (d >> 48);
    // Now do branchless select!
    s  = 64;
    // if (r > t) {s -= 32; r -= t;}
    s -= ((t - r) & 256) >> 3;
    r -= (t & ((t - r) >> 8));
    t  = (d >> (s - 16)) & 0xff;
    // if (r > t) {s -= 16; r -= t;}
    s -= ((t - r) & 256) >> 4;
    r -= (t & ((t - r) >> 8));
    t  = (c >> (s - 8)) & 0xf;
    // if (r > t) {s -= 8; r -= t;}
    s -= ((t - r) & 256) >> 5;
    r -= (t & ((t - r) >> 8));
    t  = (b >> (s - 4)) & 0x7;
    // if (r > t) {s -= 4; r -= t;}
    s -= ((t - r) & 256) >> 6;
    r -= (t & ((t - r) >> 8));
    t  = (a >> (s - 2)) & 0x3;
    // if (r > t) {s -= 2; r -= t;}
    s -= ((t - r) & 256) >> 7;
    r -= (t & ((t - r) >> 8));
    t  = (v >> (s - 1)) & 0x1;
    // if (r > t) s--;
    s -= ((t - r) & 256) >> 8;
    s = 65 - s;
    return 0;
}
*/

inline int select1(uint64_t x, std::size_t th)
{
#ifndef __BMI2__
    // Modified from: Bit Twiddling Hacks
    // https://graphics.stanford.edu/~seander/bithacks.html#SelectPosFromMSBRank
    unsigned int s;       // Output: Resulting position of bit with rank r [1-64]
    uint64_t a, b, c, d;  // Intermediate temporaries for bit count.
    unsigned int t;       // Bit count temporary.
    auto pc = popcount(x);
    assert(pc >= 0);
    assert(th < static_cast<std::size_t>(pc));
    th = pc - th;

    a = x - ((x >> 1) & ~0UL / 3);
    b = (a & ~0UL / 5) + ((a >> 2) & ~0UL / 5);
    c = (b + (b >> 4)) & ~0UL / 0x11;
    d = (c + (c >> 8)) & ~0UL / 0x101;
    t = (d >> 32) + (d >> 48);
    s = 64;
    s -= ((t - th) & 256) >> 3;
    th -= (t & ((t - th) >> 8));
    t = (d >> (s - 16)) & 0xff;
    s -= ((t - th) & 256) >> 4;
    th -= (t & ((t - th) >> 8));
    t = (c >> (s - 8)) & 0xf;
    s -= ((t - th) & 256) >> 5;
    th -= (t & ((t - th) >> 8));
    t = (b >> (s - 4)) & 0x7;
    s -= ((t - th) & 256) >> 6;
    th -= (t & ((t - th) >> 8));
    t = (a >> (s - 2)) & 0x3;
    s -= ((t - th) & 256) >> 7;
    th -= (t & ((t - th) >> 8));
    t = (x >> (s - 1)) & 0x1;
    s -= ((t - th) & 256) >> 8;
    return s - 1;
#else
    uint64_t i = 1ULL << th;
    asm("pdep %[x], %[mask], %[x]" : [x] "+r"(x) : [mask] "r"(i));
    asm("tzcnt %[bit], %[index]" : [index] "=r"(i) : [bit] "g"(x) : "cc");
    return i;
#endif
}

inline int select0(uint64_t x, std::size_t th) 
{
    return select1(~x, th);
}

// ------------------------------------- Vectors -------------------------------------------

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
    view_t const * const p = reinterpret_cast<view_t const *>(arr);
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

template <typename T>
inline std::size_t size(std::vector<T> v)
{
    return 8 * sizeof(T) * v.size();
}

} // namespace bit

#endif // BIT_OPERATIONS_HPP
