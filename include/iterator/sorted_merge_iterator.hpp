#ifndef SORTED_MERGE_ITERATOR_HPP
#define SORTED_MERGE_ITERATOR_HPP

#include <algorithm>
#include <functional>
#include <vector>
#include "standalone_iterator.hpp"

namespace iterators {

template <class SortedIterator>
class sorted_merge_iterator
{
    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = typename SortedIterator::value_type;
        using pointer           = value_type*;
        using reference         = value_type&;
        
        sorted_merge_iterator();
        sorted_merge_iterator(std::vector<standalone_iterator<SortedIterator>>& begin_end);
        value_type operator*() const;
        sorted_merge_iterator const& operator++();
        bool operator==(sorted_merge_iterator const& other) const;
        bool operator!=(sorted_merge_iterator const& other) const;

    private:
        void advance_heap_head();
        bool has_next() const noexcept;
        std::vector<standalone_iterator<SortedIterator>> iterators;
        std::vector<uint32_t> index_heap;
        std::function<bool(uint32_t, uint32_t)> heap_idx_comparator;
};

//-----------------------------------------------------------------------------

template <class SortedIterator>
sorted_merge_iterator<SortedIterator>::sorted_merge_iterator()
    : heap_idx_comparator( []([[maybe_unused]] uint32_t i, [[maybe_unused]] uint32_t j) {return false;} )
{}

template <class SortedIterator>
sorted_merge_iterator<SortedIterator>::sorted_merge_iterator(std::vector<standalone_iterator<SortedIterator>>& v)
    : heap_idx_comparator( [this](uint32_t i, uint32_t j) { return (*iterators[i] > *iterators[j]);} )
{
    uint32_t i = 0;
    for (auto itr : v) {
        if (itr.has_next()) { // filtering to remove empty iterators is needed to guarantee has_next() consistency 
            iterators.push_back(itr);
            index_heap.push_back(i++);
        }
    }
    std::make_heap(index_heap.begin(), index_heap.end(), heap_idx_comparator);
}

template <class SortedIterator>
typename sorted_merge_iterator<SortedIterator>::value_type
sorted_merge_iterator<SortedIterator>::operator*() const 
{
    return *iterators.at(index_heap.front());
}

template <class SortedIterator>
sorted_merge_iterator<SortedIterator> const& 
sorted_merge_iterator<SortedIterator>::operator++() 
{
    advance_heap_head();
    return *this;
}

template <class SortedIterator>
bool 
sorted_merge_iterator<SortedIterator>::operator==(sorted_merge_iterator const& other) const 
{
    // If comparing with end (given by default constructor), which has index_heap.size() = 0 by definition,
    // then no need to spend time checking heap and iterators one-by-one.
    if ( // use short-circuit logic to avoid heavy computations
        (index_heap.size() == other.index_heap.size()) && 
        (index_heap == other.index_heap)) { 
        for (auto idx : index_heap) {
            if (iterators.at(idx) != other.iterators.at(idx)) return false;
        }
        return true;
    }
    return false;
}

template <class SortedIterator>
bool 
sorted_merge_iterator<SortedIterator>::operator!=(sorted_merge_iterator const& other) const 
{
    return !(operator==(other));
}

template <class SortedIterator>
void 
sorted_merge_iterator<SortedIterator>::advance_heap_head() 
{
    uint32_t idx = index_heap.front();
    ++iterators[idx];
    if (iterators.at(idx).has_next()) {
        std::size_t pos = 0;
        std::size_t size = index_heap.size();
        while (2 * pos + 1 < size) {
            std::size_t i = 2 * pos + 1;
            if (i + 1 < size and heap_idx_comparator(index_heap.at(i), index_heap.at(i + 1))) ++i;
            if (heap_idx_comparator(index_heap.at(i), index_heap.at(pos))) break;
            std::swap(index_heap.at(pos), index_heap.at(i));
            pos = i;
        }
    } else {
        std::pop_heap(index_heap.begin(), index_heap.end(), heap_idx_comparator);
        index_heap.pop_back();
    }
}

template <class SortedIterator>
bool 
sorted_merge_iterator<SortedIterator>::has_next() const noexcept
{
    if (index_heap.size() == 1) {
        return iterators.at(index_heap.front()).has_next();
    } else if (index_heap.size() == 0) return false;
    return true;
}

} // namespace iterators

#endif // SORTED_MERGE_ITERATOR_HPP