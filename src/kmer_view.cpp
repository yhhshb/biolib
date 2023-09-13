#include "../include/kmer_view.hpp"
#include <limits>
#include <cassert>

namespace hash {

minimizer_position_extractor::minimizer_position_extractor(uint8_t k, uint8_t m)
    : klen(k), mlen(m)
{
    assert(k <= sizeof(uint64_t) * 8);
    assert(m <= k);
    // mask = (1ULL << (2*m)) - 1;
    if (2 * m != sizeof(mask) * 8) mask = (decltype(mask)(1) << (2 * m)) - 1;
    else mask = std::numeric_limits<decltype(mask)>::max();
}

uint8_t minimizer_position_extractor::get_k() const noexcept
{
    return klen;
}

uint8_t minimizer_position_extractor::get_m() const noexcept
{
    return mlen;
}

}