#ifndef HASH_SAMPLER_HPP
#define HASH_SAMPLER_HPP

#include <cstdint>
#include <cstddef>
#include <cassert>

namespace sampler {

template <class Iterator, typename HashFunctionFamily>
class hash_sampler
{
    public:
        class const_iterator
        {
            public:
                using iterator_category = std::forward_iterator_tag;
                using difference_type   = std::ptrdiff_t;
                using value_type        = typename Iterator::value_type;
                using pointer           = value_type*;
                using reference         = value_type&;
                
                const_iterator(hash_sampler const& sampler, Iterator const& start);
                value_type operator*() const;
                const_iterator const& operator++();
                const_iterator operator++(int);

            private:
                hash_sampler const* parent_sampler;
                Iterator itr_start;

                void find_first_kmer() noexcept;

                friend bool operator==(const_iterator const& a, const_iterator const& b) 
                {
                    bool same_parent = a.parent_sampler == b.parent_sampler;
                    bool same_start = a.itr_start == b.itr_start;
                    return same_parent and same_start;
                };
                friend bool operator!=(const_iterator const& a, const_iterator const& b) {return not (a == b);};

                template <typename T>
                T optional_unwrap(T const& val) const noexcept {return val;}

                template <typename T>
                T optional_unwrap(std::optional<T> const& opt) const noexcept {return *opt;}
        };

        hash_sampler(Iterator const& start, Iterator const& stop, HashFunctionFamily const hash, uint64_t seed, double sampling_rate);
        const_iterator cbegin() const;
        const_iterator cend() const;
        const_iterator begin() const {return cbegin();};
        const_iterator end() const {return cend();};
        double get_sampling_rate() const;

    private:
        Iterator const itr_start;
        Iterator const itr_stop;
        HashFunctionFamily const mhash;
        uint64_t mseed;
        double srate;
        typename HashFunctionFamily::hash_type threshold;

        friend bool operator==(hash_sampler const& a, hash_sampler const& b)
        {
            bool same_range = (a.itr_start == b.itr_start and a.itr_stop == b.itr_stop);
            return same_range;
        };
        friend bool operator!=(hash_sampler const& a, hash_sampler const& b) {return not (a == b);};
};

template <class Iterator, typename HashFunctionFamily>
hash_sampler<Iterator, HashFunctionFamily>::hash_sampler(Iterator const& start, Iterator const& stop, HashFunctionFamily const hash, uint64_t seed, double sampling_rate) 
    : itr_start(start), itr_stop(stop), mhash(hash), mseed(seed), srate(sampling_rate)
{
    if (srate > 1 or srate < 0) throw std::invalid_argument("[hash_sampler] Invalid sampling rate");
    threshold = static_cast<typename HashFunctionFamily::hash_type>(srate * std::numeric_limits<typename HashFunctionFamily::hash_type>::max());
}

template <class Iterator, typename HashFunctionFamily>
typename hash_sampler<Iterator, HashFunctionFamily>::const_iterator 
hash_sampler<Iterator, HashFunctionFamily>::cbegin() const
{
    return const_iterator(*this, itr_start);
}

template <class Iterator, typename HashFunctionFamily>
typename hash_sampler<Iterator, HashFunctionFamily>::const_iterator 
hash_sampler<Iterator, HashFunctionFamily>::cend() const
{
    return const_iterator(*this, itr_stop);
}

template <class Iterator, typename HashFunctionFamily>
double 
hash_sampler<Iterator, HashFunctionFamily>::get_sampling_rate() const
{
    return srate;
}

//--------------------------------------------------------------------------------------------------------------------------------------

template <class Iterator, typename HashFunctionFamily>
hash_sampler<Iterator, HashFunctionFamily>::const_iterator::const_iterator(hash_sampler const& sampler, Iterator const& start)
    : parent_sampler(&sampler), itr_start(start)
{
    find_first_kmer();
}

template <class Iterator, typename HashFunctionFamily>
typename hash_sampler<Iterator, HashFunctionFamily>::const_iterator::value_type 
hash_sampler<Iterator, HashFunctionFamily>::const_iterator::operator*() const
{
    assert(*itr_start);
    return optional_unwrap(*itr_start);
}

template <class Iterator, typename HashFunctionFamily>
typename hash_sampler<Iterator, HashFunctionFamily>::const_iterator const& 
hash_sampler<Iterator, HashFunctionFamily>::const_iterator::operator++()
{
    ++itr_start;
    find_first_kmer();
    return *this;
}

template <class Iterator, typename HashFunctionFamily>
typename hash_sampler<Iterator, HashFunctionFamily>::const_iterator 
hash_sampler<Iterator, HashFunctionFamily>::const_iterator::operator++(int)
{
    auto current = *this;
    operator++();
    return current;
}

template <class Iterator, typename HashFunctionFamily>
void
hash_sampler<Iterator, HashFunctionFamily>::const_iterator::find_first_kmer() noexcept
{
    while (itr_start != parent_sampler->itr_stop and parent_sampler->mhash(*itr_start, parent_sampler->mseed) >= parent_sampler->threshold) ++itr_start;
}

} // namespace sampler

#endif // HASH_SAMPLER_HPP