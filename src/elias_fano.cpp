#include "../include/elias_fano.hpp"

#include <iostream>

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
array::leq_find(std::size_t s) const
{
    auto hb = s >> lsb.bit_width();
    std::size_t start_idx = hb ? msbrs.select0(hb - 1) : 0;
    auto itr = const_iterator(*this, start_idx);
    for (; itr != cbegin() and *itr > s; --itr) {
        --start_idx;
    }
    if (*itr > s) --start_idx; // check begin;
    return start_idx;
}

std::size_t 
array::geq_find(std::size_t s) const
{
    auto hb = s >> lsb.bit_width();
    std::size_t start_idx = hb ? msbrs.select0(hb - 1) : 0;
    for (auto itr = const_iterator(*this, start_idx); itr != cend() and *itr < s; ++itr) {
        ++start_idx;
    }
    return start_idx;
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

array::const_iterator 
array::cbegin() const
{
    return const_iterator(*this, 0);
}

array::const_iterator 
array::cend() const
{
    return const_iterator(*this, size());
}

array::const_iterator 
array::begin() const
{
    return cbegin();
}

array::const_iterator 
array::end() const
{
    return cend();
}

array::diff_iterator
array::cdiff_begin() const
{
    return diff_iterator(*this, 0);
}

array::diff_iterator 
array::cdiff_end() const
{
    return diff_iterator(*this, size());
}

array::diff_iterator 
array::diff_begin() const
{
    return cdiff_begin();
}

array::diff_iterator 
array::diff_end() const
{
    return cdiff_end();
}

void 
array::swap(array& other) noexcept
{
    msbrs.swap(other.msbrs);
    lsb.swap(other.lsb);
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

array::const_iterator::const_iterator(array const& view, std::size_t idx)
    : const_iterator{delegate(view, idx)}
{}

array::const_iterator::const_iterator(delegate const& helper)
    : parent_view(helper.parent_view), 
      index(helper.index), 
      one_itr(bv_t::one_position_iterator(parent_view.msbrs.data(), helper.starting_position)), 
      buffered_msb(helper.buffered_msb)
{}

array::const_iterator::value_type
array::const_iterator::operator*() const noexcept
{
    auto msb = buffered_msb << parent_view.lsb.bit_width();
    auto lsb = parent_view.lsb.template at<value_type>(index);
    return msb | lsb;
}

array::const_iterator const&
array::const_iterator::operator++() noexcept
{
    // std::cerr << "index = " << index << "\n";
    if (index < parent_view.size()) {
        auto old_pos = *one_itr;
        // std::cerr << "old pos = " << old_pos << "\n";
        ++index;
        auto new_pos = *(++one_itr);
        // std::cerr << "index (incremented) = " << index << "\n";
        // std::cerr << "new pos = " << new_pos << "\n";
        // std::cerr << "buffered msb = " << buffered_msb << "\n";
        buffered_msb += new_pos - old_pos - 1;
    }
    // std::cerr << "buffered msb = " << buffered_msb << "\n";
    return *this;
}

array::const_iterator
array::const_iterator::operator++(int) noexcept
{
    auto current = *this;
    operator++();
    return current;
}

array::const_iterator const&
array::const_iterator::operator--() noexcept
{
    if (index != reverse_out_of_bound_marker) {
        --index;
        auto old_pos = *one_itr;
        --one_itr;
        buffered_msb += old_pos - *one_itr;
    }
    return *this;
}

array::const_iterator
array::const_iterator::operator--(int) noexcept
{
    auto current = *this;
    operator--();
    return current;
}

array::diff_iterator::diff_iterator(array const& view, std::size_t index)
    : itr(view, index ? index - 1 : 0)
{
    if (index == 0) prev = 0;
    else {
        prev = *itr; // save value at index - 1
        ++itr; // now it points to index
    }
}

array::diff_iterator::value_type 
array::diff_iterator::operator*() const noexcept
{
    return *itr - prev;
}

array::diff_iterator const& 
array::diff_iterator::operator++() noexcept
{
    prev = *itr;
    ++itr;
    return *this;
}

array::diff_iterator 
array::diff_iterator::operator++(int) noexcept
{
    auto current = *this;
    operator++();
    return current;
}

} // namespace ef
} // namespace bit