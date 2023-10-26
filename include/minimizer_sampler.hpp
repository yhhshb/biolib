#ifndef MINIMIZER_SAMPLER_HPP
#define MINIMIZER_SAMPLER_HPP

#include <vector>
#include <string>

#include <iostream>

namespace sampler {

template <class Iterator, typename HashFunctionFamily>
class minimizer_sampler
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

                const_iterator(minimizer_sampler const& sampler, Iterator const& start);
                value_type const& operator*() const;
                const_iterator const& operator++();
                const_iterator operator++(int);

            private:
                struct mm_pair {
                    value_type mer;
                    // typename std::result_of<HashFunctionFamily>::type hash_value;
                    typename HashFunctionFamily::hash_type hash_value;
                };

                minimizer_sampler const* parent_sampler;
                Iterator itr_start;
                std::vector<mm_pair> window;
                std::size_t widx, minpos;
                void init();
                void reset_window();
                void find_new_min();

                friend bool operator==(const_iterator const& a, const_iterator const& b) 
                {
                    bool same_parent = a.parent_sampler == b.parent_sampler
                    bool same_start = a.itr_start == b.itr_start;
                    return same_parent and same_start;
                };
                friend bool operator!=(const_iterator const& a, const_iterator const& b) {return not (a == b);};
        };
        minimizer_sampler(Iterator const& start, Iterator const& stop, HashFunctionFamily hash, uint64_t seed, uint16_t window_size);
        const_iterator cbegin() const;
        const_iterator cend() const;
        uint16_t get_w() const;

    private:
        Iterator const itr_start;
        Iterator const itr_stop;
        HashFunctionFamily mhash; // FIXME use a proper HashFunction (seeded) as template parameter
        uint64_t mseed;
        uint16_t w;

        friend bool operator==(minimizer_sampler const& a, minimizer_sampler const& b)
        {
            bool same_range = (a.itr_start == b.itr_start and a.itr_stop == b.itr_stop);
            return same_range;
        };
        friend bool operator!=(minimizer_sampler const& a, minimizer_sampler const& b) {return not (a == b);};
};

template <class Iterator, typename HashFunctionFamily>
minimizer_sampler<Iterator, HashFunctionFamily>::minimizer_sampler(Iterator const& start, Iterator const& stop, HashFunctionFamily hash, uint64_t seed, uint16_t window_size)
    : itr_start(start), itr_stop(stop), mhash(hash), mseed(seed), w(window_size)
{}

template <class Iterator, typename HashFunctionFamily>
typename minimizer_sampler<Iterator, HashFunctionFamily>::const_iterator minimizer_sampler<Iterator, HashFunctionFamily>::cbegin() const
{
    return const_iterator(*this, itr_start);
}

template <class Iterator, typename HashFunctionFamily>
typename minimizer_sampler<Iterator, HashFunctionFamily>::const_iterator minimizer_sampler<Iterator, HashFunctionFamily>::cend() const
{
    return const_iterator(*this, itr_stop);
}

template <class Iterator, typename HashFunctionFamily>
uint16_t minimizer_sampler<Iterator, HashFunctionFamily>::get_w() const
{
    return w;
}

//---------------------------------------------------------------------------------------------------------------------------------------

template <class Iterator, typename HashFunctionFamily>
minimizer_sampler<Iterator, HashFunctionFamily>::const_iterator::const_iterator(minimizer_sampler const& sampler, Iterator const& start) 
    : parent_sampler(&sampler), itr_start(start), widx(0), minpos(0)
{
    init();
}

template <class Iterator, typename HashFunctionFamily>
typename minimizer_sampler<Iterator, HashFunctionFamily>::const_iterator::value_type const& minimizer_sampler<Iterator, HashFunctionFamily>::const_iterator::operator*() const
{
    return window[minpos].mer;
}

template <class Iterator, typename HashFunctionFamily>
typename minimizer_sampler<Iterator, HashFunctionFamily>::const_iterator const& minimizer_sampler<Iterator, HashFunctionFamily>::const_iterator::operator++()
{
    value_type item = *itr_start++;
    if (not item) { // std::optional to signal sequence breaks
        reset_window();
    } else {
        bool outside = (widx == minpos);
        window[widx].mer = item; 
        window[widx].hash_value = parent_sampler->mhash(item, parent_sampler->mseed);
        if (outside) find_new_min();
        else if (window[minpos].hash_value > window[widx].hash_value) minpos = widx;
        widx = (widx + 1) % parent_sampler->w;
    }
    return *this;
}

template <class Iterator, typename HashFunctionFamily>
typename minimizer_sampler<Iterator, HashFunctionFamily>::const_iterator minimizer_sampler<Iterator, HashFunctionFamily>::const_iterator::operator++(int)
{
    auto current = *this;
    operator++();
    return current;
}

template <class Iterator, typename HashFunctionFamily>
void minimizer_sampler<Iterator, HashFunctionFamily>::const_iterator::init()
{
    window.resize(parent_sampler->w);
    reset_window();
}

template <class Iterator, typename HashFunctionFamily>
void minimizer_sampler<Iterator, HashFunctionFamily>::const_iterator::reset_window()
{
    minpos = 0;
    for (std::size_t widx = 0; itr_start != parent_sampler->itr_stop and widx < parent_sampler->w; ++widx) {
        auto item = *itr_start++;
        if (not item) {
            widx = 0;
            minpos = 0;
        } else {
            window[widx] = {*item, parent_sampler->mhash(*item, parent_sampler->mseed)};
            if (window[widx].hash_value > window[minpos].hash_value) minpos = widx;
        }
    }
}

template <class Iterator, typename HashFunctionFamily>
void minimizer_sampler<Iterator, HashFunctionFamily>::const_iterator::find_new_min()
{
    for (std::size_t i = widx, minpos = widx; i < window.size(); ++i) {
        if (window[minpos].hash_value > window[i].hash_value) minpos = i;
    }
    for (std::size_t i = 0; i < widx; ++i) {
        if (window[minpos].hash_value > window[i].hash_value) minpos = i;
    }
}

} // namespace sampler

#endif // MINIMIZER_SAMPLER_HPP