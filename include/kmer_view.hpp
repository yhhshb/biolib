#ifndef KMER_VIEW_HPP
#define KMER_VIEW_HPP

#include <string>
#include <optional>
#include <cassert>
#include <limits>

#include "constants.hpp"
#include "hash.hpp"

namespace wrapper {

template <typename KmerType>
struct kmer_context_t {
    std::optional<KmerType> value;
    std::size_t position; // position from start
    std::size_t id; // unique id for current view
};

template <typename KmerType, class Iterator>
class kmer_view
{
    public:
        class const_iterator
        {
            public:
                using kmer_context_type    = kmer_context_t<KmerType>;
                using iterator_category = std::forward_iterator_tag;
                using difference_type   = std::ptrdiff_t;
                using value_type        = kmer_context_type;
                using pointer           = value_type*;
                using reference         = value_type&;

                const_iterator(kmer_view const* view) noexcept;
                const_iterator(kmer_view const* view, int dummy_end) noexcept;
                value_type operator*() const noexcept; // FIXME avoid return by copy here if possible
                const_iterator const& operator++();
                const_iterator operator++(int);
                KmerType get_mask() const noexcept;

            private:
                kmer_view const* parent_view;
                Iterator itr;
                uint8_t strand;
                std::size_t bases_since_last_break;
                std::size_t position;
                std::size_t kmer_count; // id
                KmerType mask;
                std::size_t shift;
                KmerType kmer_buffer[2];
                void find_first_good_kmer();

                friend bool operator==(const_iterator const& a, const_iterator const& b) {return a.parent_view == b.parent_view and a.itr == b.itr;};
                friend bool operator!=(const_iterator const& a, const_iterator const& b) {return not (a == b);};
        };

        kmer_view(Iterator start, Iterator stop, uint8_t k, bool canonical = false);
        // kmer_view(char const* contig, std::size_t contig_len, uint8_t k, bool canonical = false);
        // kmer_view(std::string const& contig, uint8_t k, bool canonical = false) noexcept;
        const_iterator cbegin() const;
        const_iterator cend() const noexcept;
        const_iterator begin() const;
        const_iterator end() const noexcept;
        uint8_t get_k() const noexcept;

    private:
        // char const* seq;
        // std::size_t slen;
        Iterator itr_start;
        Iterator itr_stop;
        uint8_t klen;
        bool canon;

        friend bool operator==(kmer_view const& a, kmer_view const& b)
        {
            bool same_range = (a.itr_start == b.itr_start and a.itr_stop == b.itr_stop);
            bool same_klen = (a.klen == b.klen);
            bool same_canon = (a.canon == b.canon);
            return same_range and same_klen and same_canon;
        };
        friend bool operator!=(kmer_view const& a, kmer_view const& b) {return not (a == b);};
};

template <typename KmerType>
kmer_view<KmerType, std::string::const_iterator> kmer_view_from_string(const std::string& s, uint8_t k, bool canonical) {
    return kmer_view<KmerType, std::string::const_iterator> (s.cbegin(), s.cend(), k, canonical);
}

template <typename KmerType>
kmer_view<KmerType, char_iterator> kmer_view_from_cstr(char const* s, std::size_t len, uint8_t k, bool canonical) {
    return kmer_view<KmerType, char_iterator> (char_iterator(s), char_iterator(s + len), k, canonical);
}

//---------------------------------------------------------------------------------------------------------------------------------------------------

// template <typename KmerType>
// kmer_view<KmerType>::kmer_view(char const* contig, std::size_t contig_len, uint8_t k, bool canonical)
//     : seq(contig), slen(contig_len), klen(k), canon(canonical)
// {
//     if (not contig) throw std::runtime_error("[k-mer view] null contig");
// }

// template <typename KmerType>
// kmer_view<KmerType>::kmer_view(std::string const& contig, uint8_t k, bool canonical) noexcept
//     : seq(contig.c_str()), slen(contig.length()), klen(k), canon(canonical)
// {}

template <typename KmerType, class Iterator>
kmer_view<KmerType, Iterator>::kmer_view(Iterator start, Iterator stop, uint8_t k, bool canonical)
    : itr_start(start), itr_stop(stop), klen(k), canon(canonical)
{
    static_assert(std::is_same<typename Iterator::value_type, char>::value);
    // if (not contig) throw std::runtime_error("[k-mer view] null contig");
}

template <typename KmerType, class Iterator>
typename kmer_view<KmerType, Iterator>::const_iterator
kmer_view<KmerType, Iterator>::cbegin() const
{
    return const_iterator(this);
}

template <typename KmerType, class Iterator>
typename kmer_view<KmerType, Iterator>::const_iterator
kmer_view<KmerType, Iterator>::cend() const noexcept
{
    return const_iterator(this, 0);
}

template <typename KmerType, class Iterator>
typename kmer_view<KmerType, Iterator>::const_iterator
kmer_view<KmerType, Iterator>::begin() const
{
    return cbegin();
}

template <typename KmerType, class Iterator>
typename kmer_view<KmerType, Iterator>::const_iterator
kmer_view<KmerType, Iterator>::end() const noexcept
{
    return cend();
}

template <typename KmerType, class Iterator>
uint8_t
kmer_view<KmerType, Iterator>::get_k() const noexcept
{
    return klen;
}

//---------------------------------------------------------------------------------------------------------------------------------------------------

template <typename KmerType, class Iterator>
kmer_view<KmerType, Iterator>::const_iterator::const_iterator(kmer_view const* view, [[maybe_unused]] int dummy_end) noexcept
    : parent_view(view), itr(parent_view->itr_stop), strand(0), bases_since_last_break(0), position(0), kmer_count(0)
{
    if (parent_view->klen) shift = 2 * (parent_view->klen - 1);
}

template <typename KmerType, class Iterator>
kmer_view<KmerType, Iterator>::const_iterator::const_iterator(kmer_view const* view) noexcept
    : parent_view(view), itr(parent_view->itr_start), strand(0), bases_since_last_break(0), position(0), kmer_count(0)
{
    if (2 * parent_view->klen != sizeof(mask) * 8) mask = (KmerType(1) << (2 * parent_view->klen)) - 1;
    else mask = std::numeric_limits<decltype(mask)>::max();
    if (parent_view->klen) shift = 2 * (parent_view->klen - 1);
    find_first_good_kmer();
}

template <typename KmerType, class Iterator>
typename kmer_view<KmerType, Iterator>::const_iterator::value_type
kmer_view<KmerType, Iterator>::const_iterator::operator*() const noexcept
{
    assert(position >= parent_view->klen);
    if (bases_since_last_break == 0) return kmer_context_type{std::nullopt, position - parent_view->klen, kmer_count};
    return kmer_context_type{kmer_buffer[strand], position - parent_view->klen, kmer_count};
}

template <typename KmerType, class Iterator>
typename kmer_view<KmerType, Iterator>::const_iterator const&
kmer_view<KmerType, Iterator>::const_iterator::operator++()
{
    assert(itr != parent_view->itr_stop);
    ++kmer_count; // ids are for valid k-mer only
    if (bases_since_last_break == 0) {
        find_first_good_kmer();
        return *this;
    }
    auto c = constants::seq_nt4_table[*itr++];
    ++position;
    if (c < 4) {
        kmer_buffer[0] = (kmer_buffer[0] << 2 | c) & mask;            /* forward m-mer */
        kmer_buffer[1] = ((kmer_buffer[1] >> 2) | ((3ULL ^ c) << shift)); /* reverse m-mer */
        if (parent_view->canon and kmer_buffer[0] != kmer_buffer[1]) strand = kmer_buffer[0] < kmer_buffer[1] ? 0 : 1;  // strand, if symmetric k-mer then use previous strand
        ++bases_since_last_break;
    } else {
        bases_since_last_break = 0;
    }
    return *this;
}

template <typename KmerType, class Iterator>
typename kmer_view<KmerType, Iterator>::const_iterator
kmer_view<KmerType, Iterator>::const_iterator::operator++(int)
{
    auto res = *this;
    operator++();
    return res;
}

template <typename KmerType, class Iterator>
void
kmer_view<KmerType, Iterator>::const_iterator::find_first_good_kmer()
{
    assert(itr != parent_view->itr_stop);
    while(itr != parent_view->itr_stop and bases_since_last_break < parent_view->klen) {
        auto c = constants::seq_nt4_table.at(*itr++);
        ++position;
        if (c < 4) {
            kmer_buffer[0] = (kmer_buffer[0] << 2 | c) & mask;            /* forward m-mer */
            kmer_buffer[1] = ((kmer_buffer[1] >> 2) | (3ULL ^ c) << shift); /* reverse m-mer */
            if (parent_view->canon and kmer_buffer[0] != kmer_buffer[1]) strand = kmer_buffer[0] < kmer_buffer[1] ? 0 : 1;
            ++bases_since_last_break;
        } else {
            bases_since_last_break = 0;
        }
    }
    if (bases_since_last_break < parent_view->klen) { // deals with N's at the end of sequences, otherwise infinite loop
        bases_since_last_break = 0;
        ++itr;
    }
}

template <typename KmerType, class Iterator>
KmerType
kmer_view<KmerType, Iterator>::const_iterator::get_mask() const noexcept
{
    return mask;
}

} // namespace wrapper

namespace hash {

class minimizer_position_extractor
{
    public:
        using value_type = uint64_t;
        minimizer_position_extractor(uint8_t k, uint8_t m);
        template <typename KmerType> std::size_t operator()(wrapper::kmer_context_t<KmerType> const& kmer) const noexcept;
        uint8_t get_k() const noexcept;
        uint8_t get_m() const noexcept;

    private:
        uint8_t klen;
        uint8_t mlen;
        uint64_t mask;
        hash::hash64 hasher;
};

template <typename KmerType>
std::size_t
minimizer_position_extractor::operator()(wrapper::kmer_context_t<KmerType> const& kmer) const noexcept
{
    if (!kmer.value) return klen + 1;
    auto km = kmer.value.value();
    uint64_t mval = hasher(km & mask, 0);
    uint8_t minpos = 0;
    for (std::size_t i = 0; i < static_cast<uint8_t>(klen - mlen + 1); ++i) {
        auto val = hasher(km & mask, 0);
        if (mval >= val) {
            mval = val;
            minpos = i;
        }
        km >>= 2; // modify referenced object inside optional
    }
    return klen - mlen - minpos; // TODO check correctness
}

} // namespace hash

#endif // KMER_VIEW_HPP
