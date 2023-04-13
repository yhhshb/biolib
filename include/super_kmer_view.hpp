#ifndef SUPER_KMER_VIEW_HPP
#define SUPER_KMER_VIEW_HPP

#include <string>
#include <vector>

#include "minimizer_view.hpp"

namespace wrapper {

template <typename KmerType, typename MinimizerType, typename HashFunction>
class super_kmer_view
{
    public:
        typedef typename minimizer_view<KmerType, MinimizerType, HashFunction> mm_view_type;

        class const_iterator
        {
            public:
                struct super_kmer_t {
                    mm_view_type minimizer; // 2-bit packed minimizer
                    uint8_t mm_pos;         // position of the minimizer in the first k-mer
                    uint8_t size;           // super k-mer size (number of k-mers)
                };
                using iterator_category = std::forward_iterator_tag;
                using difference_type   = std::ptrdiff_t;
                using value_type        = super_kmer_t;
                using pointer           = value_type*;
                using reference         = value_type&;

                const_iterator(super_kmer_view const* view);
                const_iterator(super_kmer_view const* view, int dummy_end);
                super_kmer_t const& operator*() const;
                const_iterator const& operator++();
                const_iterator operator++(int);

            private:
                super_kmer_view const* parent_view;
                typename minimizer_view<KmerType, MinimizerType, HashFunction>::const_iterator start, stop, prev;
                std::size_t kmer_count;
                std::size_t prev_idx;
                super_kmer_t current_sk;

                friend bool operator==(const_iterator const& a, const_iterator const& b);
                friend bool operator!=(const_iterator const& a, const_iterator const& b);
        };

        super_kmer_view(char const* contig, std::size_t contig_len, uint8_t k, uint8_t m, bool canonical = false) noexcept;
        super_kmer_view(std::string const& contig, uint8_t k, uint8_t m, bool canonical = false) noexcept;
        const_iterator cbegin() const;
        const_iterator cend() const noexcept;
        uint8_t get_k() const noexcept;
        uint8_t get_m() const noexcept;

    private:
        minimizer_view<KmerType, MinimizerType, HashFunction> mm_view;
};

//---------------------------------------------------------------------------------------------------------------------------------------------------

template <typename KmerType, typename MinimizerType, typename HashFunction>
super_kmer_view<KmerType, MinimizerType, HashFunction>::super_kmer_view(char const* contig, std::size_t contig_len, uint8_t k, uint8_t m, bool canonical = false) noexcept
    : mm_view(contig, contig_len, k, m, canonical)
{}

template <typename KmerType, typename MinimizerType, typename HashFunction>
super_kmer_view<KmerType, MinimizerType, HashFunction>::super_kmer_view(std::string const& contig, uint8_t k, uint8_t m, bool canonical = false) noexcept
    : mm_view(contig, contig.length(), k, m, canonical)
{}

template <typename KmerType, typename MinimizerType, typename HashFunction>
super_kmer_view<KmerType, MinimizerType, HashFunction>::const_iterator super_kmer_view<KmerType, MinimizerType, HashFunction>::cbegin() const
{
    return const_iterator(this);
}

template <typename KmerType, typename MinimizerType, typename HashFunction>
super_kmer_view<KmerType, MinimizerType, HashFunction>::const_iterator super_kmer_view<KmerType, MinimizerType, HashFunction>::cend() const noexcept
{
    return const_iterator(this, 0);
}

template <typename KmerType, typename MinimizerType, typename HashFunction>
uint8_t super_kmer_view<KmerType, MinimizerType, HashFunction>::get_k() const noexcept
{
    return mm_view.get_k();
}

template <typename KmerType, typename MinimizerType, typename HashFunction>
uint8_t super_kmer_view<KmerType, MinimizerType, HashFunction>::get_m() const noexcept
{
    return mm_view.get_m();
}

//---------------------------------------------------------------------------------------------------------------------------------------------------

template <typename KmerType, typename MinimizerType, typename HashFunction>
super_kmer_view<KmerType, MinimizerType, HashFunction>::const_iterator::const_iterator(super_kmer_view const* view, int dummy_end) 
    : parent_view(view), kmer_count(0), prev_idx(0)
{
    start = stop = parent_view->mm_view.cend();
}

template <typename KmerType, typename MinimizerType, typename HashFunction>
super_kmer_view<KmerType, MinimizerType, HashFunction>::const_iterator::const_iterator(super_kmer_view const* view) 
    : parent_view(view), kmer_count(0), prev_idx(0)
{
    if (parent_view->slen < parent_view->klen) throw std::runtime_error("Unable to initialize super-k-mer iterator on sequence of length " + std::to_string(parent_view->klen) + "with k = " + std::to_string(parent_view->klen) + " and m = " + std::to_string(parent_view->mlen));
    start = parent_view->cbegin();
    stop = parent_view->cend();
    operator++();
}

template <typename KmerType, typename MinimizerType, typename HashFunction>
super_kmer_view<KmerType, MinimizerType, HashFunction>::const_iterator::super_kmer_t const& 
super_kmer_view<KmerType, MinimizerType, HashFunction>::const_iterator::operator*() const
{
    return current_sk;
}

template <typename KmerType, typename MinimizerType, typename HashFunction>
super_kmer_view<KmerType, MinimizerType, HashFunction>::const_iterator const& 
super_kmer_view<KmerType, MinimizerType, HashFunction>::const_iterator::operator++()
{
    assert(parent_view);
    assert(char_idx <= parent_view->slen);

    current_sk.minimizer = *start;
    ++start;
    std::size_t next_idx = (start == stop) ? parent_view->slen : (*start).mm_idx;
    if ((next_idx - prev_idx) > (2 * parent_view->get_k() - m + 1)) throw std::runtime_error("Bases different than ACGT are not yet supported");
    current_sk.mm_pos = current_sk.minimizer.mm_idx - prev_idx; // position of minimizer in first k-mer of the super k-mer
    current_sk.size = next_idx - prev_idx - parent_view->get_k() + 1; // number of k-mers in super k-mer
    return *this;
}

template <typename KmerType, typename MinimizerType, typename HashFunction>
super_kmer_view<KmerType, MinimizerType, HashFunction>::const_iterator
super_kmer_view<KmerType, MinimizerType, HashFunction>::const_iterator::operator++(int)
{
    auto res = *this;
    operator++();
    return res;
}

template <typename K, typename M, typename H>
bool operator==(typename super_kmer_view<K,M,H>::const_iterator const& a, typename super_kmer_view<K,M,H>::const_iterator const& b)
{
    return a.parent_view == b.parent_view and a.start == b.start;
}

template <typename K, typename M, typename H>
bool operator!=(typename super_kmer_view<K,M,H>::const_iterator const& a, typename super_kmer_view<K,M,H>::const_iterator const& b)
{
    return not (a == b);
}

} // namespace wrapper

#endif // SUPER_KMER_VIEW_HPP