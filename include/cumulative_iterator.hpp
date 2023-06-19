#ifndef CUMULATIVE_ITERATOR_HPP
#define CUMULATIVE_ITERATOR_HPP

#include <cstddef>

namespace iterators {

template <typename Iterator, typename T = std::size_t>
class cumulative_iterator 
{
    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = T;
        using pointer           = value_type*;
        using reference         = value_type&;

        cumulative_iterator(Iterator other) 
            : inc(true), itr(other), cumulative_sum(0) 
        {}

        value_type operator*() 
        {
            if (inc) {
                cumulative_sum += *itr;
                inc = false;
            }
            return cumulative_sum;
        }

        cumulative_iterator const& operator++() noexcept
        {
            if (inc) cumulative_sum += *itr;
            else inc = true;
            ++itr;
            return *this;
        }

        cumulative_iterator operator++(int) noexcept
        {
            auto current = *this;
            operator++();
            return current;
        }

    private:
        bool inc;
        Iterator itr;
        value_type cumulative_sum;

        friend bool operator==(cumulative_iterator const& a, cumulative_iterator const& b) 
        {
            bool same_position = a.itr == b.itr;
            // bool same_inc = a.inc == b.inc;
            // bool same_sum = a.cumulative_sum == b.cumulative_sum;
            return same_position;
        }
        friend bool operator!=(cumulative_iterator const& a, cumulative_iterator const& b) { return not (a == b); }
};

} // namespace iterators

#endif // CUMULATIVE_ITERATOR_HPP