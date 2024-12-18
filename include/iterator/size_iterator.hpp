#ifndef SIZE_ITERATOR_HPP
#define SIZE_ITERATOR_HPP

namespace iterators {

template <class Iterator>
class size_iterator
{
    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = typename Iterator::value_type;
        using pointer           = value_type*;
        using reference         = value_type&;

        size_iterator(Iterator start, std::size_t start_idx) noexcept 
            : itr(start), idx(start_idx) {}

        value_type operator*() const 
        {
            return *itr;
        }

        size_iterator const& operator++() 
        {
            ++itr; 
            ++idx;
            return *this;
        }
        
        size_iterator operator++(int) 
        {
            auto current = *this; 
            operator++();
            return current;
        }

        std::size_t get_idx() const noexcept 
        {
            return idx;
        }

    private:
        Iterator itr;
        std::size_t idx;
        friend bool operator==(size_iterator const& a, size_iterator const& b) {return a.idx == b.idx;}
        friend bool operator!=(size_iterator const& a, size_iterator const& b) {return not (a == b);}
};

} // namespace iterators

#endif // SIZE_ITERATOR_HPP
