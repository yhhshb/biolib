#ifndef ELIAS_FANO_HPP
#define ELIAS_FANO_HPP

#include "bit_vector.hpp"
#include "packed_vector.hpp"
#include "rank_select.hpp"

namespace bit {
namespace ef {

class array
{
    public:
        template <class Iterator>
        array(Iterator start, Iterator stop);

        template <class Iterator>
        array(Iterator start, std::size_t n);

        template <class Iterator>
        array(Iterator start, std::size_t n, std::size_t u);

        std::size_t at(std::size_t idx) const; // access prefix-sum
        std::size_t diff_at(std::size_t idx) const; // access difference
        std::size_t size() const noexcept;
        std::size_t bit_size() const noexcept;

        template <class Visitor>
        void visit(Visitor& visitor) const;

    protected:
        using rs_t = rs::array<bit::vector<max_width_native_type>, 8 * sizeof(max_width_native_type), 8, true>;
        std::tuple<std::size_t, std::size_t> prev_and_at(std::size_t idx) const;

    private:
        rs_t msbrs;
        packed::vector<max_width_native_type> lsb;

        template <class Iterator>
        void build(Iterator start, std::size_t n, std::size_t u);

        template <class Iterator>
        void build(Iterator start, std::size_t n, std::random_access_iterator_tag);

        template <class Iterator>
        void build(Iterator start, Iterator stop, std::random_access_iterator_tag);

        template <class Iterator>
        void build(Iterator start, std::size_t n, std::forward_iterator_tag);

        template <class Iterator>
        void build(Iterator start, Iterator stop, std::forward_iterator_tag);
};

template <class Iterator>
array::array(Iterator start, Iterator stop)
    : lsb(packed::dynamic::vector<max_width_native_type>(0))
{
    static_assert(not std::numeric_limits<Iterator::value_type>::is_signed, "[Elias-Fano] sequence must be unsigned");
    if (start == stop) return;
    typedef typename std::iterator_traits<Iterator>::iterator_category category;
    build(start, stop, category);
}

template <class Iterator>
array::array(Iterator start, std::size_t n)
    : lsb(packed::dynamic::vector<max_width_native_type>(0))
{
    static_assert(not std::numeric_limits<Iterator::value_type>::is_signed, "[Elias-Fano] sequence must be unsigned");
    if (not n) return;
    typedef typename std::iterator_traits<Iterator>::iterator_category category;
    build(start, n, category());
}

template <class Iterator>
array::array(Iterator start, std::size_t n, std::size_t u)
    : lsb(packed::dynamic::vector<max_width_native_type>(0))
{
    static_assert(not std::numeric_limits<Iterator::value_type>::is_signed, "[Elias-Fano] sequence must be unsigned");
    if (not n) return;
    build(start, n, u);
}

std::size_t 
array::at(std::size_t idx) const
{
    if (idx < size()) throw std::out_of_range("[Elias-Fano] index out of range");
    return ((msbrs.select1(idx) - idx) << lsb.bit_width()) | lsb.template at<uint64_t>(idx);
}

std::tuple<std::size_t, std::size_t> 
array::prev_and_at(std::size_t idx) const
{
    if (idx < size()) throw std::out_of_range("[Elias-Fano] index out of range");
    auto low1 = lsb.template at<uint64_t>(idx);
    auto low2 = lsb.template at<uint64_t>(idx + 1);
    auto l = lsb.bit_width();
    auto pos = msbrs.select1(idx);
    uint64_t h1 = pos - idx;
    uint64_t h2 = *(++(msbrs.data().cpos_begin() + pos + 1)) - idx - 1; // just search for the next one, faster than calling select1 again
    auto val1 = (h1 << l) | low1;
    auto val2 = (h2 << l) | low2;
    return {val1, val2};
}

std::size_t
array::diff_at(std::size_t idx) const
{
    auto [val1, val2] = prev_and_at(idx);
    return val2 - val1;
}

template <class Iterator>
void
array::build(Iterator start, std::size_t n, std::size_t u)
{
    ++n; // add 0 at the beginning for convenience
    const uint64_t l = uint64_t((n && u / n) ? pthash::util::msb(u / n) : 0);
    const uint64_t low_mask = (uint64_t(1) << l) - 1;

    vector<max_width_native_type> msb;
    msb.resize(n + (u >> l) + 1, false);
    msb.set(0);
    packed::dynamic::vector loclsb(l);
    loclsb.reserve(n);
    if (l) loclsb.push_back(0);
    --n; // restore true size

    uint64_t prev = 0; // for convenience we added a virtual 0 at the beginning, so the first item is always 0.
    for (size_t i = 0; i < n; ++i, ++start) {
        auto v = *start;
        if (i and v < prev) throw std::runtime_error("ef_sequence is not sorted");
        if (l) loclsb.push_back(v & low_mask);
        msb.set((v >> l) + i + 1); // +1 because n+1 before
        prev = v;
    }
    rs_t temp(std::move(msb));
    msbrs.swap(temp);
    lsb.swap(loclsb);
}

template <class Iterator>
void 
array::build(Iterator start, std::size_t n, std::random_access_iterator_tag)
{
    std::size_t u = *(start + n - 1);
    build(start, n, u);
}

template <class Iterator>
void 
array::build(Iterator start, Iterator stop, std::random_access_iterator_tag) 
{
    typedef typename std::iterator_traits<Iterator>::iterator_category category;
    std::size_t n = stop - start;
    build(start, n, category());
}

template <class Iterator>
void 
array::build(Iterator start, std::size_t n, std::forward_iterator_tag)
{
    auto itr = start;
    std::advance(itr, n-1);
    std::size_t u = *itr;
    build(start, n, u);
}

template <class Iterator>
void 
array::build(Iterator start, Iterator stop, std::forward_iterator_tag) 
{
    typedef typename std::iterator_traits<Iterator>::iterator_category category;
    std::size_t u = *start;
    std::size_t n = 0;
    for (auto itr = start; itr != stop; ++itr, ++n) {
        assert(*itr >= 0);
        u = std::max(u, *itr);
    }
    build(start, n, category());
}

} // namespace ef
} // namespace bit

#endif // ELIAS_FANO_HPP