#ifndef APPEND_ITERATOR_HPP
#define APPEND_ITERATOR_HPP

#include <cinttypes>
#include <cstddef>
#include <vector>
#include "standalone_iterator.hpp"

namespace iterators {

template <class Iterator>
class append_iterator 
{
    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = typename Iterator::value_type;
        using pointer           = value_type*;
        using reference         = value_type&;

        append_iterator() noexcept {}

        append_iterator(std::vector<standalone_iterator<Iterator>>& v) noexcept
            : itr_idx(0), iterators(v)
        {
            search_for_start();
        }

        append_iterator(std::vector<standalone_iterator<Iterator>>&& v) noexcept
            : itr_idx(0), iterators(v)
        {
            search_for_start();
        }

        append_iterator const& operator++() 
        {
            ++iterators[itr_idx];
            if (iterators.at(itr_idx).has_next()) {
                search_for_start();
            }
            return *this;
        }

        append_iterator operator++(int)
        {
            auto current = *this; 
            operator++();
            return current;
        }

        typename Iterator::value_type operator*() const 
        {
            return *(iterators[itr_idx].first);
        }

        bool operator==(append_iterator const& other) const 
        {
            if (itr_idx == other.itr_idx) return iterators.at(itr_idx) == other.iterators.at(itr_idx); 
            return false;
        }

        bool operator!=(append_iterator const& other) const 
        {
            return !(operator==(other));
        }

    private:
        std::size_t itr_idx;
        std::vector<standalone_iterator<Iterator>>& iterators;

        std::size_t size() const noexcept
        {
            return iterators.size();
        }

        bool has_next() const
        {
            if (itr_idx == iterators.size()) return false;
            return true;
        }

        void search_for_start() noexcept
        {
            while (itr_idx < iterators.size() and not iterators.at(itr_idx).has_next()) ++itr_idx;
        }
};

} // namespace iterator

#endif // APPEND_ITERATOR_HPP