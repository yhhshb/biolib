#ifndef MINIMIZER_VIEW_HPP
#define MINIMIZER_VIEW_HPP

#include "constants.hpp"

namespace wrapper {

template <typename KmerType, typename MinimizerType, typename HashFunction>
class minimizer_view
{
    public:
        class const_iterator
        {
            public:
                struct minimizer_t {
                    MinimizerType minimizer; // 2-bit packed minimizer
                    uint32_t mm_idx;         // minimizer index in contig
                    uint32_t id;             // minimizer id in contig
                };
                using iterator_category = std::forward_iterator_tag;
                using difference_type   = std::ptrdiff_t;
                using value_type        = minimizer_t;
                using pointer           = value_type*;
                using reference         = value_type&;

                const_iterator(minimizer_view const* view);
                const_iterator(minimizer_view const* view, int dummy_end);
                minimizer_t const& operator*() const noexcept;
                const_iterator const& operator++();
                const_iterator operator++(int);
                std::size_t break_offset() const noexcept;
            
            private:
                class window
                {
                    public:
                        struct mm_context_t : public minimizer_t {
                            typename std::result_of<HashFunction>::type mm_hash;
                        };

                        window(std::size_t maximum_size) noexcept;
                        bool push_back(mm_context_t elem);
                        bool pop_back();
                        bool pop_front();
                        mm_context_t const& front() const;
                        mm_context_t const& back() const;
                        mm_context_t const& min() const;
                        std::size_t maximum_size() const noexcept;
                        std::size_t size() const noexcept;
                        std::size_t min_shift() const noexcept;
                        void clear() noexcept;
                    
                    private:
                        std::vector<mm_context_t> buffer;
                        std::size_t first, last;
                        std::size_t min_idx;
                        void find_new_min() const;
                };

                minimizer_view const* parent_view;
                std::size_t char_idx;
                MinimizerType mask;
                std::size_t shift;
                uint8_t strand;
                std::size_t bases_since_last_break;
                std::size_t mmer_count;
                MinimizerType mm_forward_reverse[2];
                window buffer;
                std::size_t read_base();
                void init_window();

                friend bool operator==(const_iterator const& a, const_iterator const& b) {
                    return a.parent_view == b.parent_view and a.char_idx == b.char_idx;
                }

                friend bool operator!=(const_iterator const& a, const_iterator const& b) {
                    return not (a == b);
                }
        };

        minimizer_view(char const* contig, std::size_t contig_len, uint8_t k, uint8_t m, bool canonical = false) noexcept;
        minimizer_view(std::string const& contig, uint8_t k, uint8_t m, bool canonical = false) noexcept;
        const_iterator cbegin() const;
        const_iterator cend() const noexcept;
        uint8_t get_k() const noexcept;
        uint8_t get_m() const noexcept;

    private:
        char const* seq;
        std::size_t slen;
        uint8_t klen;
        uint8_t mlen;
        bool canon;
};

//---------------------------------------------------------------------------------------------------------------------------------------------------

template <typename KmerType, typename MinimizerType, typename HashFunction>
minimizer_view<KmerType, MinimizerType, HashFunction>::const_iterator::window::window(std::size_t maximum_size) noexcept
{
    clear();
    buffer.resize(maximum_size);
}

template <typename KmerType, typename MinimizerType, typename HashFunction>
bool 
minimizer_view<KmerType, MinimizerType, HashFunction>::const_iterator::window::push_back(mm_context_t elem)
{
    bool changed = false;
    buffer.at(last) = elem;
    if (buffer.at(last) < buffer.at(min_idx)) {
        min_idx = last;
        changed = true;
    }
    ++last;
    last = ++last % buffer.size();
    return changed;
}

template <typename KmerType, typename MinimizerType, typename HashFunction>
bool 
minimizer_view<KmerType, MinimizerType, HashFunction>::const_iterator::window::pop_back()
{
    bool recompute = false;
    if (min_idx == last) recompute = true; 
    if (last == 0) last = buffer.size();
    --last;
    if (recompute) find_new_min();
    return recompute;
}

template <typename KmerType, typename MinimizerType, typename HashFunction>
bool 
minimizer_view<KmerType, MinimizerType, HashFunction>::const_iterator::window::pop_front()
{
    bool recompute = false;
    if (first == min_idx) recompute = true;
    first = (first + 1) % buffer.size();
    if (recompute) find_new_min();
    return recompute;
}

template <typename KmerType, typename MinimizerType, typename HashFunction>
typename minimizer_view<KmerType, MinimizerType, HashFunction>::const_iterator::window::mm_context_t const& 
minimizer_view<KmerType, MinimizerType, HashFunction>::const_iterator::window::front() const
{
    return buffer.at(first);
}

template <typename KmerType, typename MinimizerType, typename HashFunction>
typename minimizer_view<KmerType, MinimizerType, HashFunction>::const_iterator::window::mm_context_t const& 
minimizer_view<KmerType, MinimizerType, HashFunction>::const_iterator::window::back() const
{
    return buffer.at(last);
}

template <typename KmerType, typename MinimizerType, typename HashFunction>
typename minimizer_view<KmerType, MinimizerType, HashFunction>::const_iterator::window::mm_context_t const& 
minimizer_view<KmerType, MinimizerType, HashFunction>::const_iterator::window::min() const
{
    return buffer.at(min_idx);
}

template <typename KmerType, typename MinimizerType, typename HashFunction>
std::size_t 
minimizer_view<KmerType, MinimizerType, HashFunction>::const_iterator::window::maximum_size() const noexcept
{
    return buffer.size();
}

template <typename KmerType, typename MinimizerType, typename HashFunction>
std::size_t 
minimizer_view<KmerType, MinimizerType, HashFunction>::const_iterator::window::size() const noexcept
{
    if (first <= last) return last - first;
    return buffer.size() - (first - last);
}

template <typename KmerType, typename MinimizerType, typename HashFunction>
std::size_t 
minimizer_view<KmerType, MinimizerType, HashFunction>::const_iterator::window::min_shift() const noexcept
{
    if (first <= min_idx) return min_idx - first;
    return buffer.size() - (first - min_idx);
}

template <typename KmerType, typename MinimizerType, typename HashFunction>
void 
minimizer_view<KmerType, MinimizerType, HashFunction>::const_iterator::window::clear() noexcept
{
    first = last = 0;
    min_idx = 0;
}

template <typename KmerType, typename MinimizerType, typename HashFunction>
void 
minimizer_view<KmerType, MinimizerType, HashFunction>::const_iterator::window::find_new_min() const
{
    min_idx = first;
    for (std::size_t i = first; i != last; ++i) {
        i %= buffer.size();
        if (buffer.at(i) < buffer.at(min_idx)) min_idx = i;
    }
}

//---------------------------------------------------------------------------------------------------------------------------------------------------

template <typename KmerType, typename MinimizerType, typename HashFunction>
minimizer_view<KmerType, MinimizerType, HashFunction>::minimizer_view(char const* contig, std::size_t contig_len, uint8_t k, uint8_t m, bool canonical = false) noexcept
    : seq(contig), slen(contig_len), klen(k), mlen(m), canon(canonical)
{}

template <typename KmerType, typename MinimizerType, typename HashFunction>
minimizer_view<KmerType, MinimizerType, HashFunction>::minimizer_view(std::string const& contig, uint8_t k, uint8_t m, bool canonical = false) noexcept
    : seq(contig.c_str()), slen(contig.length()), klen(k), mlen(m), canon(canonical)
{}

template <typename KmerType, typename MinimizerType, typename HashFunction>
minimizer_view<KmerType, MinimizerType, HashFunction>::const_iterator 
minimizer_view<KmerType, MinimizerType, HashFunction>::cbegin() const
{
    return const_iterator(this);
}

template <typename KmerType, typename MinimizerType, typename HashFunction>
minimizer_view<KmerType, MinimizerType, HashFunction>::const_iterator 
minimizer_view<KmerType, MinimizerType, HashFunction>::cend() const noexcept
{
    return const_iterator(this, 0);
}

template <typename KmerType, typename MinimizerType, typename HashFunction>
uint8_t 
minimizer_view<KmerType, MinimizerType, HashFunction>::get_k() const noexcept
{
    return klen;
}

template <typename KmerType, typename MinimizerType, typename HashFunction>
uint8_t 
minimizer_view<KmerType, MinimizerType, HashFunction>::get_m() const noexcept
{
    return mlen;
}

//---------------------------------------------------------------------------------------------------------------------------------------------------

template <typename KmerType, typename MinimizerType, typename HashFunction>
minimizer_view<KmerType, MinimizerType, HashFunction>::const_iterator::const_iterator(minimizer_view const* view, int dummy_end)
    : parent_view(view), char_idx(view->slen), strand(0), bases_since_last_break(0), mmer_count(0), buffer(0)
{}

template <typename KmerType, typename MinimizerType, typename HashFunction>
minimizer_view<KmerType, MinimizerType, HashFunction>::const_iterator::const_iterator(minimizer_view const* view) 
    : parent_view(view), char_idx(0), strand(0), bases_since_last_break(0), mmer_count(0), buffer(view->klen - view->mlen + 1)
{
    if (parent_view->slen < parent_view->klen) throw std::runtime_error("Unable to initialize super-k-mer iterator on sequence of length " + std::to_string(parent_view->klen) + "with k = " + std::to_string(parent_view->klen) + " and m = " + std::to_string(parent_view->mlen));
    shift = 2 * (parent_view->mlen - 1);
    mask = (KmerType(1) << (2 * parent_view->mlen)) - 1;
    mm_forward_reverse = {MinimizerType(0), MinimizerType(0)};
    init_window();
}

template <typename KmerType, typename MinimizerType, typename HashFunction>
minimizer_view<KmerType, MinimizerType, HashFunction>::const_iterator::minimizer_t const& 
minimizer_view<KmerType, MinimizerType, HashFunction>::const_iterator::operator*() const noexcept
{
    return buffer.min();
}

template <typename KmerType, typename MinimizerType, typename HashFunction>
minimizer_view<KmerType, MinimizerType, HashFunction>::const_iterator const& 
minimizer_view<KmerType, MinimizerType, HashFunction>::const_iterator::operator++()
{
    assert(parent_view);
    assert(char_idx <= parent_view->slen);

    if (char_idx < parent_view->slen) {
        bool new_min = false;
        do {
            auto idx = read_base();
            if (bases_since_last_break >= parent_view->mlen) [[likely]] {
                typename window::mm_context_t ctx = {
                    {
                        mm_forward_reverse[strand], // minimizer
                        idx,
                        mmer_count
                    },
                    HashFunction(mm_forward_reverse[strand]) // hash
                };
                new_min = buffer.push_back(ctx);
            } else [[unlikely]] {
                assert(new_min = false);
            }
        } while(not new_min);
    }
    return *this;
}

template <typename KmerType, typename MinimizerType, typename HashFunction>
minimizer_view<KmerType, MinimizerType, HashFunction>::const_iterator
minimizer_view<KmerType, MinimizerType, HashFunction>::const_iterator::operator++(int)
{
    minimizer_t res = *this;
    operator++();
    return res;
}

template <typename KmerType, typename MinimizerType, typename HashFunction>
std::size_t 
minimizer_view<KmerType, MinimizerType, HashFunction>::const_iterator::break_offset() const noexcept
{
    return bases_since_last_break;
}

template <typename KmerType, typename MinimizerType, typename HashFunction>
std::size_t 
minimizer_view<KmerType, MinimizerType, HashFunction>::const_iterator::read_base()
{
    std::size_t mm_contig_idx = char_idx - parent_view->mlen + 1;
    auto c = constants::seq_nt4_table.at(static_cast<uint8_t>(parent_view->seq[char_idx++]));
    if (c < 4) [[likely]] {
        mm_forward_reverse[0] = (mm_forward_reverse[0] << 2 | c) & mask;            /* forward m-mer */
        mm_forward_reverse[1] = (mm_forward_reverse[1] >> 2) | (3ULL ^ c) << shift; /* reverse m-mer */
        if (parent_view->canon && mm_forward_reverse[0] != mm_forward_reverse[1]) strand = mm_forward_reverse[0] < mm_forward_reverse[1] ? 0 : 1;  // if symmetric, use previous strand
        ++bases_since_last_break;
    } else [[unlikely]] {
        bases_since_last_break = 0;
        buffer.clear();
    }
    return mm_contig_idx;
}

template <typename KmerType, typename MinimizerType, typename HashFunction>
void 
minimizer_view<KmerType, MinimizerType, HashFunction>::const_iterator::init_window()
{
    while(buffer.size() < buffer.maximum_size() and char_idx < parent_view->slen) {
        auto idx = read_base();
        if (bases_since_last_break >= parent_view->mlen) {
            typename window::mm_context_t ctx = {
                {
                    mm_forward_reverse[strand], // minimizer
                    idx, // mm_idx
                    mmer_count // id
                },
                HashFunction(mm_forward_reverse[strand]) // hash
            };
            buffer.push_back(ctx);
            ++mmer_count;
        }
    }
}

} // namespace wrapper

#endif // MINIMIZER_VIEW_HPP