#include "../include/elias_fano.hpp"

namespace bit {
namespace ef {

std::size_t 
array::at(std::size_t idx) const
{
    if (idx < size()) throw std::out_of_range("[Elias-Fano] index out of range");
    return ((msbrs.select1(idx) - idx) << lsb.bit_width()) | lsb.template at<uint64_t>(idx);
}

std::size_t
array::diff_at(std::size_t idx) const
{
    auto [val1, val2] = prev_and_at(idx);
    return val2 - val1;
}

std::size_t 
array::size() const noexcept
{
    return lsb.size() - 1;
}

std::size_t 
array::bit_size() const noexcept
{
    return lsb.bit_size() + msbrs.bit_size();
}

std::tuple<std::size_t, std::size_t> 
array::prev_and_at(std::size_t idx) const
{
    if (idx >= size()) throw std::out_of_range("[Elias-Fano] index out of range");
    // std::cerr << "[begin] prev_and_at:" << "\n";
    // std::cerr << "idx = " << idx << "\n";
    auto low1 = lsb.template at<uint64_t>(idx);
    auto low2 = lsb.template at<uint64_t>(idx + 1);
    auto l = lsb.bit_width();
    // std::cerr << "low1 = " << low1 << ", low2 = " << low2 << ", bit width = " << l << "\n";
    auto pos = msbrs.select1(idx);
    // std::cerr << "pos = " << pos << "\n";
    uint64_t h1 = pos - idx;
    uint64_t h2 = *(++(msbrs.data().cpos_begin() + pos)) - idx - 1; // just search for the next one, faster than calling select1 again
    // std::cerr << "high1 = " << h1 << ", high2 = " << h2 << "\n"; 
    auto val1 = (h1 << l) | low1;
    auto val2 = (h2 << l) | low2;
    // std::cerr << "[end] prev_and_at:" << "\n";
    return {val1, val2};
}

bool operator==(array const& a, array const& b) 
{
    bool same_msb = a.msbrs == b.msbrs;
    bool same_lsb = a.lsb == b.lsb;
    return same_msb and same_lsb;
}

bool operator!=(array const& a, array const& b) 
{
    return not (a == b);
}

} // namespace ef
} // namespace bit