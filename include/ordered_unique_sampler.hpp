#ifndef ORDERED_UNIQUE_SAMPLER_HPP
#define ORDERED_UNIQUE_SAMPLER_HPP

#include <vector>
#include <optional>
#include <mutex>

namespace sampler {

template <class Iterator>
class ordered_unique_sampler
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

                const_iterator(ordered_unique_sampler const& sampler, Iterator const& start);
                value_type const& operator*() const;
                const_iterator const& operator++();
                const_iterator operator++(int);

            private:
                // struct mm_pair {
                //     value_type mer;
                //     typename std::result_of<HashFunction>::type hash_value;
                // };

                ordered_unique_sampler const& parent_sampler;
                Iterator itr_start;
                value_type buffer;
                std::size_t unique_count;

                friend bool operator==(const_iterator const& a, const_iterator const& b) 
                {
                    bool same_start = a.itr_start == b.itr_start;
                    return (a.parent_sampler == b.parent_sampler) and same_start;
                };
                friend bool operator!=(const_iterator const& a, const_iterator const& b) {return not (a == b);};
        };

        ordered_unique_sampler(Iterator const& start, Iterator const& stop);
        const_iterator cbegin() const;
        const_iterator cend() const;
        const_iterator begin() const;
        const_iterator end() const;
        std::optional<std::size_t> size() const;

    private:
        Iterator const itr_start;
        Iterator const itr_stop;
        mutable std::optional<std::size_t> _size;
        mutable std::mutex size_guard;

        friend bool operator==(ordered_unique_sampler const& a, ordered_unique_sampler const& b)
        {
            bool same_range = (a.itr_start == b.itr_start and a.itr_stop == b.itr_stop);
            return same_range;
        };
        friend bool operator!=(ordered_unique_sampler const& a, ordered_unique_sampler const& b) {return not (a == b);};
};

template <class Iterator>
ordered_unique_sampler<Iterator>::ordered_unique_sampler(Iterator const& start, Iterator const& stop)
    : itr_start(start), itr_stop(stop), _size(std::nullopt)
{}

template <class Iterator>
typename ordered_unique_sampler<Iterator>::const_iterator ordered_unique_sampler<Iterator>::cbegin() const
{
    return const_iterator(*this, itr_start);
}

template <class Iterator>
typename ordered_unique_sampler<Iterator>::const_iterator ordered_unique_sampler<Iterator>::cend() const
{
    return const_iterator(*this, itr_stop);
}

template <class Iterator>
typename ordered_unique_sampler<Iterator>::const_iterator ordered_unique_sampler<Iterator>::begin() const
{
    return cbegin();
}

template <class Iterator>
typename ordered_unique_sampler<Iterator>::const_iterator ordered_unique_sampler<Iterator>::end() const
{
    return cend();
}

template <class Iterator>
std::optional<std::size_t> ordered_unique_sampler<Iterator>::size() const
{
    return _size;
}

template <class Iterator>
ordered_unique_sampler<Iterator>::const_iterator::const_iterator(ordered_unique_sampler const& sampler, Iterator const& start) 
    : parent_sampler(sampler), itr_start(start), unique_count(0)
{}

template <class Iterator>
typename ordered_unique_sampler<Iterator>::const_iterator::value_type const& ordered_unique_sampler<Iterator>::const_iterator::operator*() const
{
    return *itr_start;
}

template <class Iterator>
typename ordered_unique_sampler<Iterator>::const_iterator const& ordered_unique_sampler<Iterator>::const_iterator::operator++()
{
    auto prev = *itr_start;
    bool inc = false;
    while(itr_start != parent_sampler.itr_stop and prev == *itr_start) {
        ++itr_start;
        inc = true;
    }
    if (inc) ++unique_count;
    if (itr_start == parent_sampler.itr_stop) {
        std::lock_guard<std::mutex> size_update(parent_sampler.size_guard);
        parent_sampler._size = unique_count;
    }
    return *this;
}

template <class Iterator>
typename ordered_unique_sampler<Iterator>::const_iterator ordered_unique_sampler<Iterator>::const_iterator::operator++(int)
{
    auto current = *this;
    operator++();
    return current;
}

} // namespace sampler

#endif // ORDERED_UNIQUE_SAMPLER_HPP