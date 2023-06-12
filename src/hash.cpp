#include "../include/hash.hpp"
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

std::size_t minimizer_position_extractor::operator()(std::optional<uint64_t> kmer) const noexcept
{
    if (!kmer) return klen + 1;
    uint64_t mval = hasher(kmer.value() & mask, 0);
    uint8_t minpos = 0;
    for (std::size_t i = 0; i < static_cast<uint8_t>(klen - mlen + 1); ++i) {
        auto val = hasher(kmer.value() & mask, 0);
        if (mval >= val) {
            mval = val;
            minpos = i;
        }
        *kmer >>= 2; // modify referenced object inside optional
    }
    return klen - mlen - minpos; // TODO check correctness
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