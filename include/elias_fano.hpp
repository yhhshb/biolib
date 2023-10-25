#ifndef ELIAS_FANO_HPP
#define ELIAS_FANO_HPP

#include "bit_vector.hpp"
#include "packed_vector.hpp"
#include "rank_select.hpp"

#include <iostream>

namespace bit {
namespace ef {

class array
{
    protected:
        using bv_t = bit::vector<max_width_native_type>;
        using rs_t = rs::array<bv_t, 8 * sizeof(max_width_native_type), 8, true, true>;
        using pv_t = packed::vector<max_width_native_type>;
        using build_t = std::tuple<rs_t, pv_t, std::size_t>;

        std::tuple<std::size_t, std::size_t> prev_and_at(std::size_t idx) const;

    public:
        class const_iterator
        {
            public:
                using iterator_category = std::bidirectional_iterator_tag;
                using difference_type   = std::ptrdiff_t;
                using value_type        = std::size_t;
                using pointer           = value_type*;
                using reference         = value_type&;

                const_iterator(array const& view, std::size_t idx);
                value_type operator*() const noexcept;
                const_iterator const& operator++() noexcept;
                const_iterator operator++(int) noexcept;

                const_iterator const& operator--() noexcept;
                const_iterator operator--(int) noexcept;

                // std::size_t get_idx() const noexcept {return index;}
            
            private:
                struct delegate {
                    array const& parent_view;
                    std::size_t index;
                    std::size_t starting_position;
                    std::size_t buffered_msb;
                    delegate(array const& view, std::size_t idx);
                };
                static const value_type reverse_out_of_bound_marker = std::numeric_limits<value_type>::max();
                array const& parent_view;
                std::size_t index;
                bv_t::one_position_iterator one_itr;
                std::size_t buffered_msb;

                const_iterator(delegate const& helper);
                friend bool operator==(const_iterator const& a, const_iterator const& b) 
                {
                    bool same_parent = &a.parent_view == &b.parent_view;
                    bool same_index = a.index == b.index;
                    return same_parent and same_index;
                };
                friend bool operator!=(const_iterator const& a, const_iterator const& b) {return not (a == b);};
                friend difference_type operator-(const_iterator const& a, const_iterator const& b)
                {
                    bool same_parent = &a.parent_view == &b.parent_view;
                    if (not same_parent) throw std::runtime_error("[Elias-Fano const_iterator] difference between two un-related iterators");
                    return a.index - b.index;
                }
        };

        class diff_iterator
        {
            public:
                using iterator_category = std::forward_iterator_tag;
                using difference_type   = std::ptrdiff_t;
                using value_type        = std::size_t;
                using pointer           = value_type*;
                using reference         = value_type&;

                diff_iterator(array const& view, std::size_t idx);
                value_type operator*() const noexcept;
                diff_iterator const& operator++() noexcept;
                diff_iterator operator++(int) noexcept;
            
            private:
                const_iterator itr;
                value_type prev;
                friend bool operator==(diff_iterator const& a, diff_iterator const& b) 
                {
                    bool same_itr = a.itr == b.itr;
                    if (same_itr and a.prev != b.prev) throw std::runtime_error("[prev iterator] not equal despite pointing to the same position");
                    return same_itr;
                };
                friend bool operator!=(diff_iterator const& a, diff_iterator const& b) {return not (a == b);};
        };

        array() : msbrs(bv_t(0)), lsb(0) {}

        template <class Iterator>
        array(Iterator start, Iterator stop) : array(this->build(start, stop)) {}

        template <class Iterator>
        array(Iterator start, std::size_t n) : array(this->build(start, n)) {}

        template <class Iterator>
        array(Iterator start, std::size_t n, std::size_t u) : array(this->build(start, n, u)) {}

        array(array const& other) noexcept = default;
        array(array&& other) noexcept = default;
        array& operator=(array const&) noexcept = default;
        array& operator=(array&&) noexcept = default;

        std::size_t at(std::size_t idx) const; // access prefix-sum
        std::size_t diff_at(std::size_t idx) const; // access difference
        std::size_t lt_find(std::size_t s, bool ignore_duplicates = false) const; // find the index of the largest element < s
        std::size_t gt_find(std::size_t s, bool ignore_duplicates = false) const; // find the index of the smallest element > s
        std::size_t size() const noexcept;
        std::size_t bit_size() const noexcept;

        const_iterator cbegin() const;
        const_iterator cend() const;
        const_iterator begin() const;
        const_iterator end() const;

        diff_iterator cdiff_begin() const;
        diff_iterator cdiff_end() const;
        diff_iterator diff_begin() const;
        diff_iterator diff_end() const;

        void swap(array& other) noexcept;

        template <class Visitor>
        void visit(Visitor& visitor) const;

        template <class Visitor>
        void visit(Visitor& visitor);

        template <class Loader>
        static array load(Loader& visitor);

    private:
        rs_t msbrs;
        pv_t lsb;
        std::size_t _size;

        array(build_t pack) : msbrs(std::get<0>(pack)), lsb(std::get<1>(pack)), _size(std::get<2>(pack)) {} // dummy constructor for const members
        std::size_t lsb_at(std::size_t idx) const;

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

        friend bool operator==(array const& a, array const& b);
        friend bool operator!=(array const& a, array const& b);
};

#define BUILD_T std::tuple<rs::array<bit::vector<max_width_native_type>, 8 * sizeof(max_width_native_type), 8, true, true>, packed::vector<max_width_native_type>, std::size_t> 

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
    if (l) {
        loclsb.reserve(n);
        loclsb.push_back(0);
    }
    --n; // restore true size

    uint64_t prev = 0; // for convenience we added a virtual 0 at the beginning, so the first item is always 0.
    for (size_t i = 0; i < n; ++i, ++start) {
        auto v = *start;
        if (i and v < prev) throw std::runtime_error("ef_sequence is not sorted");
        if (l) loclsb.push_back(v & low_mask);
        msb.set((v >> l) + i + 1); // +1 because n+1 before
        prev = v;
    }
    if (l) assert(loclsb.size() == (n + (l ? 1 : 0)));
    rs_t temp(std::move(msb));
    return std::make_tuple(temp, loclsb, n);
}

template <class Iterator>
BUILD_T
array::build(Iterator start, std::size_t n, std::random_access_iterator_tag)
{
    std::size_t u = *(start + (n - 1));
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
    // std::cerr << "n = " << n << ", u = " << u << "\n";
    return build(start, n, std::forward_iterator_tag());
}

template <class Iterator>
BUILD_T
array::build(Iterator start, Iterator stop)
{
    typedef typename std::iterator_traits<Iterator>::iterator_category category;
    return build(start, stop, category());
}

template <class Visitor>
void 
array::visit(Visitor& visitor) const
{
    visitor.visit(msbrs);
    visitor.visit(lsb);
    visitor.visit(_size);
}

template <class Visitor>
void 
array::visit(Visitor& visitor)
{
    visitor.visit(msbrs);
    visitor.visit(lsb);
    visitor.visit(_size);
}

template <class Loader>
array 
array::load(Loader& visitor)
{
    array r;
    r.msbrs = decltype(r.msbrs)::load(visitor);
    r.lsb = decltype(r.lsb)::load(visitor);
    return r;
}

#undef BUILD_T

} // namespace ef
} // namespace bit

#endif // ELIAS_FANO_HPP