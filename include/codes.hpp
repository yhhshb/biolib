#ifndef CODES_HPP
#define CODES_HPP

#include "bit_operations.hpp"
#include "bit_vector.hpp"
#include "bit_parser.hpp"
#include <cassert>

namespace bit {

template <typename T>
inline std::size_t binary_bitsize(T x) { return msbll(x) + 1; }

template <typename T>
inline std::size_t unary_bitsize(T x) { return static_cast<std::size_t>(x); }

template <typename T>
inline std::size_t gamma_bitsize(T x) 
{
    if (x == std::numeric_limits<T>::max()) throw std::overflow_error("[gamma_bitsize] Unable to compute size of gamma encoding");
    auto b = binary_bitsize(x + 1);
    return unary_bitsize(b) + b - 1;
}

template <typename T>
inline std::size_t delta_bitsize(T x) 
{
    if (x == std::numeric_limits<T>::max()) throw std::overflow_error("[delta_bitsize] Unable to compute size of gamma encoding");
    auto b = binary_bitsize(x + 1);
    return gamma_bitsize(b - 1) + b - 1;
}

namespace encoder {

template <typename UnsignedIntegerType1, typename UnsignedIntegerType2, std::size_t width>
static inline void fixed_width(::bit::vector<UnsignedIntegerType1>& out, UnsignedIntegerType2 x) 
{
    if constexpr (width > 8 * sizeof(x)) throw std::length_error("[fixed width] Encoding exceeding type width");
    out.push_back(x, width);
}

template <typename UnsignedIntegerType>
static inline void unary(::bit::vector<UnsignedIntegerType>& out, std::size_t x) 
{
    while(x >= ::bit::size<UnsignedIntegerType>()) {
        out.push_back(UnsignedIntegerType(0), ::bit::size<UnsignedIntegerType>());
    }
    UnsignedIntegerType last = UnsignedIntegerType(1) << x;
    if (x + 1 > ::bit::size<UnsignedIntegerType>()) throw std::runtime_error("[unary encoder] This should never happen");
    out.push_back(last, x + 1);
}

template <typename UnsignedIntegerType>
static inline void binary(::bit::vector<UnsignedIntegerType>& out, UnsignedIntegerType x, std::size_t k)
{
    assert(k > 0);
    assert(x <= k);
    std::size_t b = msbll(k) + 1;
    out.push_back(x, b); // write the integer x <= k using b = ceil(log2(k+1)) bits 
}

template <typename UnsignedIntegerType>
static inline void gamma(::bit::vector<UnsignedIntegerType>& out, std::size_t x) 
{
    // if (x == std::numeric_limits<UnsignedIntegerType2>::max()) throw std::runtime_error("[gamma] Unable to gamma encode");
    auto xx = x + 1;
    std::size_t b = msbll(xx);
    unary(out, b);
    auto mask = (static_cast<std::size_t>(1) << b) - 1;
    out.push_back(xx & mask, b);
}

template <typename UnsignedIntegerType>
static inline void delta(::bit::vector<UnsignedIntegerType>& out, std::size_t x) 
{
    // if (x == std::numeric_limits<UnsignedIntegerType2>::max()) throw std::runtime_error("[gamma] Unable to gamma encode");
    auto xx = x + 1;
    std::size_t b = msbll(xx);
    gamma(out, b);
    auto mask = (static_cast<std::size_t>(1) << b) - 1;
    out.push_back(xx & mask, b);
}

template <typename UnsignedIntegerType>
static inline void rice(::bit::vector<UnsignedIntegerType>& out, std::size_t x, const std::size_t k) 
{
    assert(k > 0);
    auto q = x >> k;
    auto r = x - (q << k);
    gamma(out, q);
    out.push_back(r, k);
}

} // namespace encoder

namespace decoder {

template <typename UnsignedIntegerType, std::size_t width>
static inline UnsignedIntegerType fixed_width(parser<UnsignedIntegerType>& parsr)
{
    return parsr.parse_fixed(width);
}

template <typename UnsignedIntegerType>
static inline std::size_t unary(parser<UnsignedIntegerType>& parsr)
{ 
    return parsr.parse_0();
}

template <typename UnsignedIntegerType>
static inline UnsignedIntegerType binary(parser<UnsignedIntegerType>& parsr, std::size_t k) 
{
    assert(k > 0);
    std::size_t b = msbll(k) + 1;
    auto x = parsr.parse_fixed(b);
    assert(x <= k);
    return x; // read b=ceil(log2(r+1)) bits and interprets them as the integer x
}

template <typename UnsignedIntegerType1, typename UnsignedIntegerType2>
static inline std::size_t gamma(parser<UnsignedIntegerType1>& parsr) 
{
    std::size_t b = unary(parsr);
    return (static_cast<std::size_t>(parsr.parse_fixed(b)) | (std::size_t(1) << b)) - 1;
}

template <typename UnsignedIntegerType>
static inline std::size_t delta(parser<UnsignedIntegerType>& parsr) 
{
    std::size_t b = gamma(parsr);
    return (parsr.parse_fixed(b) | (std::size_t(1) << b)) - 1;
}

template <typename UnsignedIntegerType>
static inline std::size_t rice(parser<UnsignedIntegerType>& parsr, const uint64_t k) 
{
    assert(k > 0);
    auto q = gamma(it);
    auto r = parsr.parse_fixed(k);
    return r + (q << k);
}

} // namespace decoder

} // namespace bit

#endif // CODES_HPP