#ifndef APPEND_ITERATOR_HPP
#define APPEND_ITERATOR_HPP

#include <cinttypes>
#include <cstddef>
#include <vector>

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

        append_iterator(std::vector<std::pair<Iterator, Iterator>>& start_end) noexcept
            : i(0), iterators(start_end) {search_for_start();}

        append_iterator(std::vector<std::pair<Iterator, Iterator>>&& start_end) noexcept
            : i(0), iterators(start_end) {search_for_start();}

        void operator++() 
        {
            ++(iterators[i].first);
            if (iterators[i].first == iterators[i].second) {
                search_for_start();
                // ++i;
            }
        }

        typename Iterator::value_type operator*() const 
        {
            return *(iterators[i].first);
        }

        bool has_next() const 
        { // FIXME This is not very C++-ish, rewrite the class as a dummy container with methods begin() and end()
            if (i == iterators.size()) return false;
            return true;
        }

        std::size_t size() const
        {
            return iterators.size();
        }

    private:
        uint64_t i;
        std::vector<std::pair<Iterator, Iterator>>& iterators;

        void search_for_start() noexcept
        {
            while (i < iterators.size() and iterators.at(i).first == iterators.at(i).second) ++i;
        }
};

} // namespace iterator

#endif // APPEND_ITERATOR_HPP