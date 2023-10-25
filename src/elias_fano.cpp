#include "../include/elias_fano.hpp"

namespace bit {
namespace ef {

std::size_t 
array::at(std::size_t idx) const
{
    if (idx >= size()) throw std::out_of_range("[Elias-Fano] index out of range");
    ++idx;
    return ((msbrs.select1(idx) - idx) << lsb.bit_width()) | lsb_at(idx);
}

std::size_t
array::diff_at(std::size_t idx) const
{
    auto [val1, val2] = prev_and_at(idx);
    return val2 - val1;
}

std::size_t
array::lt_find(std::size_t s, bool ignore_duplicates) const // ignore duplicates on the cumulative sum
{
    auto hb = s >> lsb.bit_width();
    // auto start_idx = hb ? msbrs.select1(hb - 1) - (hb - 1) : 0; // TODO remove start idx
    auto itr = const_iterator(*this, hb ? msbrs.select1(hb - 1) - (hb - 1) : 0);
    auto start = begin();
    auto stop = cend();
    std::pair<std::size_t, std::size_t> val_run = {*itr, 0};
    while (itr != start and *itr == val_run.first) {
        --itr; // we need to sync the beginning of the current run
    }
    val_run = {-1, 0};
    do {
        if (itr != stop) val_run.first = *itr;
        val_run.second = 0;
        while(*itr == val_run.first) { // count run length of equal values
            // std::cerr << "*itr = " << *itr << "current value = " << val_run.first << "\n";
            ++itr;
            ++val_run.second;
        }
    } while (*itr < s and itr != stop);
    assert(itr == stop or *itr >= s);
    // std::cerr << "idx = " << itr - start << ", run length = " << val_run.second << "\n";
    return (itr - start) - (ignore_duplicates ? val_run.second : 1); // if ignoring duplicates, return the index at the beginning of the run
}

std::size_t
array::gt_find(std::size_t s, bool ignore_duplicates) const // ignore duplicates on the cumulative sum, not the difference
{
    auto hb = s >> lsb.bit_width();
    auto start_idx = hb ? msbrs.select1(hb) - hb : 0;
    auto itr = const_iterator(*this, start_idx);
    auto stop = cend();
    for (; *itr <= s and itr != stop; ++itr) {
        ++start_idx;
    }
    if (ignore_duplicates and itr != stop) {
        auto gt = *itr;
        auto prev = itr;
        ++itr; // start_idx is not the same as (itr - cbegin()) because of this line
        for (; *itr == gt and itr != stop; ++itr) {
            ++prev;
            ++start_idx;
        }
    }
    return start_idx;
}

std::size_t 
array::size() const noexcept
{
    // return lsb.size() - 1;
    return _size;
}

std::size_t 
array::bit_size() const noexcept
{
    return lsb.bit_size() + msbrs.bit_size() + bit::size(_size);
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
    auto low1 = lsb_at(idx);
    auto low2 = lsb_at(idx + 1);
    auto l = lsb.bit_width();
    auto pos = msbrs.select1(idx);
    uint64_t h1 = pos - idx;
    uint64_t h2 = *(++(msbrs.data().cpos_begin() + pos)) - idx - 1; // just search for the next one, faster than calling select1 again
    auto val1 = (h1 << l) | low1;
    auto val2 = (h2 << l) | low2;
    return {val1, val2};
}

std::size_t 
array::lsb_at(std::size_t idx) const
{
    if (lsb.bit_width()) return lsb.template at<std::size_t>(idx);
    return 0;
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

array::const_iterator::delegate::delegate(array const& view, std::size_t idx) 
    : parent_view(view), index(idx + 1), starting_position(0), buffered_msb(0)
{
    if (idx > parent_view.size()) throw std::out_of_range("[Elias-Fano] iterator out of range");
    else if (idx < parent_view.size()) {
        starting_position = parent_view.msbrs.select1(index);
        buffered_msb = starting_position - index;
    } else {
        // idx == size() --> cend // nothing to do
    }
}

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
    auto lsb = parent_view.lsb_at(index);
    return msb | lsb;
}

array::const_iterator const&
array::const_iterator::operator++() noexcept
{
    // std::cerr << "index = " << index << "\n";
    if (index < parent_view.size()+1) {
        auto old_pos = *one_itr;
        // std::cerr << "[++] old position = " << old_pos << " ";
        ++index;
        auto new_pos = *(++one_itr);
        // std::cerr << ", new position = " << new_pos << "\n";
        buffered_msb += (new_pos - old_pos) - 1;
    }
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
        // std::cerr << "[--] old position = " << old_pos << " ";
        --one_itr;
        // std::cerr << ", new position = " << *one_itr << "\n";
        buffered_msb -= (old_pos - *one_itr) - 1;
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