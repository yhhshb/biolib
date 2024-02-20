#ifndef MINIMIZER_VIEW_HPP
#define MINIMIZER_VIEW_HPP

#include <type_traits>
#include <array>
#include <string>
#include "constants.hpp"

namespace wrapper {

#define CLASS_HEADER template <typename KmerType, typename MinimizerType, typename HashFunction, typename Iterator>
#define METHOD_HEADER minimizer_view<KmerType, MinimizerType, HashFunction, Iterator>

CLASS_HEADER
class minimizer_view
{
    public:
        class const_iterator
        {
            public:
                struct minimizer_context_t {
                    MinimizerType value; // 2-bit packed minimizer
                    std::size_t position; // minimizer index from start
                    std::size_t id; // unique id for current view
                };
                using iterator_category = std::forward_iterator_tag;
                using difference_type   = std::ptrdiff_t;
                using value_type        = minimizer_context_t;
                using pointer           = value_type*;
                using reference         = value_type&;

                const_iterator(minimizer_view const* view);
                const_iterator(minimizer_view const* view, int dummy_end);
                value_type const& operator*() const noexcept;
                const_iterator const& operator++();
                const_iterator operator++(int);
                std::size_t break_offset() const noexcept;
            
            private:
                class window
                {
                    public:
                        struct mm_context_t : public minimizer_context_t {
                            typename std::invoke_result<HashFunction, MinimizerType, uint64_t>::type mm_hash; // ATTENTION: uint64_t = seed type
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
                Iterator itr;
                uint8_t strand;
                std::size_t bases_since_last_break;
                std::size_t position;
                std::size_t mmer_count;
                window buffer;
                MinimizerType mask;
                std::size_t shift;
                std::array<MinimizerType, 2> mm_forward_reverse;
                std::size_t read_base();
                void init_window();

                friend bool operator==(const_iterator const& a, const_iterator const& b) {
                    return a.parent_view == b.parent_view and a.itr == b.itr;
                }

                friend bool operator!=(const_iterator const& a, const_iterator const& b) {
                    return not (a == b);
                }
        };

        minimizer_view(Iterator start, Iterator stop, uint8_t k, uint8_t m, uint64_t seed, bool canonical = false) noexcept;
        const_iterator cbegin() const;
        const_iterator cend() const noexcept;
        uint8_t get_k() const noexcept;
        uint8_t get_m() const noexcept;

    private:
        Iterator itr_start;
        Iterator itr_stop;
        uint8_t klen;
        uint8_t mlen;
        uint64_t mseed;
        bool canon;
};

template <typename KmerType, typename MmerType, typename HashFunction>
minimizer_view<KmerType, MmerType, HashFunction, std::string::const_iterator> minimizer_view_from_string(
    const std::string& s, 
    uint8_t k, 
    uint8_t m,
    uint64_t seed,  
    bool canonical) {
    return minimizer_view<KmerType, MmerType, HashFunction, std::string::const_iterator> (s.cbegin(), s.cend(), k, m, seed, canonical);
}

template <typename KmerType, typename MmerType, typename HashFunction>
minimizer_view<KmerType, MmerType, HashFunction, char_iterator> minimizer_view_from_cstr(
    char const* s, 
    std::size_t len, 
    uint8_t k, 
    uint8_t m,
    uint64_t seed,  
    bool canonical) {
    return minimizer_view<KmerType, MmerType, HashFunction, char_iterator> (char_iterator(s), char_iterator(s + len), k, m, seed, canonical);
}

//---------------------------------------------------------------------------------------------------------------------------------------------------

CLASS_HEADER
METHOD_HEADER::minimizer_view(Iterator start, Iterator stop, uint8_t k, uint8_t m, uint64_t seed, bool canonical) noexcept
    : itr_start(start), itr_stop(stop), klen(k), mlen(m), mseed(seed), canon(canonical)
{}

CLASS_HEADER
typename METHOD_HEADER::const_iterator 
METHOD_HEADER::cbegin() const
{
    return const_iterator(this);
}

CLASS_HEADER
typename METHOD_HEADER::const_iterator 
METHOD_HEADER::cend() const noexcept
{
    return const_iterator(this, 0);
}

CLASS_HEADER
uint8_t 
METHOD_HEADER::get_k() const noexcept
{
    return klen;
}

CLASS_HEADER
uint8_t 
METHOD_HEADER::get_m() const noexcept
{
    return mlen;
}

//---------------------------------------------------------------------------------------------------------------------------------------------------

CLASS_HEADER
METHOD_HEADER::const_iterator::const_iterator(minimizer_view const* view, int dummy_end)
    : parent_view(view), itr(parent_view->itr_stop), strand(0), bases_since_last_break(0), position(0), mmer_count(0), buffer(0)
{}

CLASS_HEADER
METHOD_HEADER::const_iterator::const_iterator(minimizer_view const* view) 
    : parent_view(view), itr(parent_view->itr_start), strand(0), bases_since_last_break(0), position(0), mmer_count(0), buffer(view->klen - view->mlen + 1)
{
    // if (parent_view->slen < parent_view->klen) throw std::runtime_error("Unable to initialize super-k-mer iterator on sequence of length " + std::to_string(parent_view->klen) + "with k = " + std::to_string(parent_view->klen) + " and m = " + std::to_string(parent_view->mlen));
    shift = 2 * (parent_view->mlen - 1);
    mask = (KmerType(1) << (2 * parent_view->mlen)) - 1;
    mm_forward_reverse = {MinimizerType(0), MinimizerType(0)};
    init_window();
}

CLASS_HEADER
typename METHOD_HEADER::const_iterator::value_type const& 
METHOD_HEADER::const_iterator::operator*() const noexcept
{
    return buffer.min();
}

CLASS_HEADER
typename METHOD_HEADER::const_iterator const& 
METHOD_HEADER::const_iterator::operator++()
{
    assert(parent_view);
    assert(itr != parent_view->itr_stop);

    if (itr != parent_view->itr_stop) {
        bool new_min = false;
        do {
            auto idx = read_base();
            if (bases_since_last_break >= parent_view->mlen) [[likely]] {
                typename window::mm_context_t ctx = {
                    {
                        mm_forward_reverse[strand], // minimizer value
                        idx, // position
                        mmer_count // id
                    },
                    HashFunction::hash(mm_forward_reverse[strand], parent_view->mseed) // hash
                };
                new_min = buffer.push_back(ctx);
            } else [[unlikely]] {
                assert(new_min = false);
            }
        } while(not new_min);
    }
    return *this;
}

CLASS_HEADER
typename METHOD_HEADER::const_iterator
METHOD_HEADER::const_iterator::operator++(int)
{
    auto res = *this;
    operator++();
    return res;
}

CLASS_HEADER
std::size_t 
METHOD_HEADER::const_iterator::break_offset() const noexcept
{
    return bases_since_last_break;
}

CLASS_HEADER
std::size_t 
METHOD_HEADER::const_iterator::read_base()
{
    auto c = constants::seq_nt4_table.at(static_cast<uint8_t>(*itr++));
    ++position;
    if (c < 4) [[likely]] {
        mm_forward_reverse[0] = (mm_forward_reverse[0] << 2 | c) & mask;            /* forward m-mer */
        mm_forward_reverse[1] = (mm_forward_reverse[1] >> 2) | (3ULL ^ c) << shift; /* reverse m-mer */
        if (parent_view->canon && mm_forward_reverse[0] != mm_forward_reverse[1]) strand = mm_forward_reverse[0] < mm_forward_reverse[1] ? 0 : 1;  // if symmetric, use previous strand
        ++bases_since_last_break;
    } else [[unlikely]] {
        bases_since_last_break = 0;
        buffer.clear();
    }
    return position - parent_view->mlen + 1;
}

CLASS_HEADER
void 
METHOD_HEADER::const_iterator::init_window()
{
    while(buffer.size() < buffer.maximum_size() and itr != parent_view->itr_stop) {
        auto idx = read_base();
        if (bases_since_last_break >= parent_view->mlen) {
            typename window::mm_context_t ctx = {
                {
                    mm_forward_reverse[strand], // minimizer value
                    idx, // position
                    mmer_count // id
                },
                HashFunction::hash(mm_forward_reverse[strand], parent_view->mseed) // hash
            };
            buffer.push_back(ctx);
            ++mmer_count;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------------------------------------

CLASS_HEADER
METHOD_HEADER::const_iterator::window::window(std::size_t maximum_size) noexcept
{
    clear();
    buffer.resize(maximum_size);
}

CLASS_HEADER
bool 
METHOD_HEADER::const_iterator::window::push_back(mm_context_t elem)
{
    bool changed = false;
    buffer.at(last) = elem;
    if (buffer.at(last).mm_hash < buffer.at(min_idx).mm_hash) {
        min_idx = last;
        changed = true;
    }
    ++last;
    last = ++last % buffer.size();
    return changed;
}

CLASS_HEADER
bool 
METHOD_HEADER::const_iterator::window::pop_back()
{
    bool recompute = false;
    if (min_idx == last) recompute = true; 
    if (last == 0) last = buffer.size();
    --last;
    if (recompute) find_new_min();
    return recompute;
}

CLASS_HEADER
bool 
METHOD_HEADER::const_iterator::window::pop_front()
{
    bool recompute = false;
    if (first == min_idx) recompute = true;
    first = (first + 1) % buffer.size();
    if (recompute) find_new_min();
    return recompute;
}

CLASS_HEADER
typename METHOD_HEADER::const_iterator::window::mm_context_t const& 
METHOD_HEADER::const_iterator::window::front() const
{
    return buffer.at(first);
}

CLASS_HEADER
typename METHOD_HEADER::const_iterator::window::mm_context_t const& 
METHOD_HEADER::const_iterator::window::back() const
{
    return buffer.at(last);
}

CLASS_HEADER
typename METHOD_HEADER::const_iterator::window::mm_context_t const& 
METHOD_HEADER::const_iterator::window::min() const
{
    return buffer.at(min_idx);
}

CLASS_HEADER
std::size_t 
METHOD_HEADER::const_iterator::window::maximum_size() const noexcept
{
    return buffer.size();
}

CLASS_HEADER
std::size_t 
METHOD_HEADER::const_iterator::window::size() const noexcept
{
    if (first <= last) return last - first;
    return buffer.size() - (first - last);
}

CLASS_HEADER
std::size_t 
METHOD_HEADER::const_iterator::window::min_shift() const noexcept
{
    if (first <= min_idx) return min_idx - first;
    return buffer.size() - (first - min_idx);
}

CLASS_HEADER
void 
METHOD_HEADER::const_iterator::window::clear() noexcept
{
    first = last = 0;
    min_idx = 0;
}

CLASS_HEADER
void 
METHOD_HEADER::const_iterator::window::find_new_min() const
{
    min_idx = first;
    for (std::size_t i = first; i != last; ++i) {
        i %= buffer.size();
        if (buffer.at(i).mm_hash < buffer.at(min_idx).mm_hash) min_idx = i;
    }
}

#undef CLASS_HEADER
#undef METHOD_HEADER

} // namespace wrapper

#endif // MINIMIZER_VIEW_HPP