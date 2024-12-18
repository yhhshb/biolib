#ifndef COUNTING_ITERATOR_HPP
#define COUNTING_ITERATOR_HPP

namespace iterators {

template <typename T>
class counting_iterator
{
    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = T;
        using pointer           = value_type*;
        using reference         = value_type&;
        counting_iterator(T val) : mval(val) {}
        T operator*() const noexcept {return mval;}
        counting_iterator const& operator++() {++mval; return *this;}
        counting_iterator operator++(int) {auto current = *this; ++mval; return current;}

    private:
        T mval;
        friend bool operator==(counting_iterator const& a, counting_iterator const& b) {return a.mval == b.mval;}
        friend bool operator!=(counting_iterator const& a, counting_iterator const& b) {return not (a == b);}
};

} // namespace iterators

#endif // COUNTING_ITERATOR_HPP