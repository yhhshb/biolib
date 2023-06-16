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
        using bv_t = bit::vector<max_width_native_type>;
        using rs_t = rs::array<bv_t, 8 * sizeof(max_width_native_type), 8, true>;
        using pv_t = packed::vector<max_width_native_type>;
        using build_t = std::pair<rs_t, pv_t>;

        std::tuple<std::size_t, std::size_t> prev_and_at(std::size_t idx) const;

    private:
        rs_t msbrs;
        pv_t lsb;

        template <class Iterator>
        static build_t build(Iterator start, std::size_t n, std::size_t u); // main construction function

        template <class Iterator>
        static build_t build(Iterator start, std::size_t n, std::random_access_iterator_tag);

        template <class Iterator>
        static build_t build(Iterator start, std::size_t n, std::forward_iterator_tag);

        template <class Iterator>
        static build_t build(Iterator start, std::size_t n);

        template <class Iterator>
        static build_t build(Iterator start, Iterator stop, std::random_access_iterator_tag);

        template <class Iterator>
        static build_t build(Iterator start, Iterator stop, std::forward_iterator_tag);

        template <class Iterator>
        static build_t build(Iterator start, Iterator stop);

        array(build_t pack) : msbrs(pack.first), lsb(pack.second) {} // dummy constructor for const members
};

template <class Iterator>
array::array(Iterator start, Iterator stop)
    : array(this->build(start, stop))
    //msbrs(bv_t()), lsb(packed::vector<max_width_native_type>(0))
{
    // static_assert(not std::numeric_limits<typename Iterator::value_type>::is_signed, "[Elias-Fano] sequence must be unsigned");
    // typedef typename std::iterator_traits<Iterator>::iterator_category category;
    // build(start, stop, category());
}

template <class Iterator>
array::array(Iterator start, std::size_t n)
    : array(this->build(start, n))
    //lsb(packed::vector<max_width_native_type>(0))
{
    // static_assert(not std::numeric_limits<typename Iterator::value_type>::is_signed, "[Elias-Fano] sequence must be unsigned");
    // if (not n) return;
    // typedef typename std::iterator_traits<Iterator>::iterator_category category;
    // build(start, n, category());
}

template <class Iterator>
array::array(Iterator start, std::size_t n, std::size_t u)
    : array(this->build(start, n, u))
    //lsb(packed::vector<max_width_native_type>(0))
{
    // static_assert(not std::numeric_limits<typename Iterator::value_type>::is_signed, "[Elias-Fano] sequence must be unsigned");
    // if (not n) return;
    // build(start, n, u);
}

#define BUILD_T std::pair<rs::array<bit::vector<max_width_native_type>, 8 * sizeof(max_width_native_type), 8, true>, packed::vector<max_width_native_type>> 

template <class Iterator>
BUILD_T
array::build(Iterator start, std::size_t n, std::size_t u)
{
    static_assert(not std::numeric_limits<typename Iterator::value_type>::is_signed, "[Elias-Fano] sequence must be unsigned");
    ++n; // add 0 at the beginning for convenience
    const std::size_t l = (n && u / n) ? static_cast<std::size_t>(std::ceil(std::log2(u / n))) : 0;
    const max_width_native_type low_mask = (static_cast<max_width_native_type>(1) << l) - 1;

    vector<max_width_native_type> msb;
    msb.resize(n + (u >> l) + 1, false);
    msb.set(0);
    packed::vector loclsb(l);
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
    return std::make_pair(temp, loclsb);
}

template <class Iterator>
BUILD_T
array::build(Iterator start, std::size_t n, std::random_access_iterator_tag)
{
    std::size_t u = *(start + n - 1);
    return build(start, n, u);
}

template <class Iterator>
BUILD_T
array::build(Iterator start, std::size_t n, std::forward_iterator_tag)
{
    auto itr = start;
    std::advance(itr, n-1);
    std::size_t u = *itr;
    return build(start, n, u);
}

template <class Iterator>
BUILD_T
array::build(Iterator start, std::size_t n)
{
    typedef typename std::iterator_traits<Iterator>::iterator_category category;
    return build(start, n, category());
}

template <class Iterator>
BUILD_T
array::build(Iterator start, Iterator stop, std::random_access_iterator_tag) 
{
    std::size_t n = stop - start;
    return build(start, n, std::random_access_iterator_tag());
}

template <class Iterator>
BUILD_T
array::build(Iterator start, Iterator stop, std::forward_iterator_tag) 
{
    std::size_t u = *start;
    std::size_t n = 0;
    for (auto itr = start; itr != stop; ++itr, ++n) {
        assert(*itr >= 0);
        u = std::max(u, *itr);
    }
    return build(start, n, std::forward_iterator_tag());
}

template <class Iterator>
BUILD_T
array::build(Iterator start, Iterator stop)
{
    typedef typename std::iterator_traits<Iterator>::iterator_category category;
    return build(start, stop, category());
}

#undef BUILD_T

} // namespace ef
} // namespace bit

#endif // ELIAS_FANO_HPP