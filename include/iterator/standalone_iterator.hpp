#ifndef STANDALONE_ITERATOR_HPP
#define STANDALONE_ITERATOR_HPP

namespace iterators {

template <class Iterator>
class standalone_iterator
{
    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = typename Iterator::value_type;
        using pointer           = value_type*;
        using reference         = value_type&;

        standalone_iterator(Iterator begin, Iterator end);
        value_type operator*() const;
        standalone_iterator const& operator++();
        standalone_iterator operator++(int);
        bool has_next() const noexcept;

        bool operator==(standalone_iterator const& other) const noexcept;
        bool operator!=(standalone_iterator const& other) const noexcept;
    
    private:
        Iterator itr;
        Iterator stop;
};

namespace standalone {

template <class Container>
standalone_iterator<typename Container::const_iterator> 
const_from(Container& ds) {
    return standalone_iterator(ds.cbegin(), ds.cend());
}

template <class Container>
standalone_iterator<typename Container::iterator> 
from(Container& ds) {
    return standalone_iterator(ds.begin(), ds.end());
}

} // namespace standalone

//-----------------------------------------------------------------------------

template <class Iterator>
standalone_iterator<Iterator>::standalone_iterator(Iterator begin, Iterator end)
    : itr(begin), stop(end)
{}

template <class Iterator>
typename standalone_iterator<Iterator>::value_type
standalone_iterator<Iterator>::operator*() const
{
    return *itr;
}

template <class Iterator>
standalone_iterator<Iterator> const&
standalone_iterator<Iterator>::operator++()
{
    ++itr;
    return *this;
}

template <class Iterator>
standalone_iterator<Iterator>
standalone_iterator<Iterator>::operator++(int) 
{
    auto current = *this; 
    operator++();
    return current;
}

template <class Iterator>
bool 
standalone_iterator<Iterator>::operator==(standalone_iterator const& other) const noexcept
{
    return itr == other.itr;
}

template <class Iterator>
bool 
standalone_iterator<Iterator>::operator!=(standalone_iterator const& other) const noexcept
{
    return !(operator==(other));
}

template <class Iterator>
bool
standalone_iterator<Iterator>::has_next() const noexcept
{
    return itr != stop;
}

} // namespace iterators

#endif // STANDALONE_ITERATOR_HPP