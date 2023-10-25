#ifndef RLE_VIEW_HPP
#define RLE_VIEW_HPP

#include <utility>

namespace wrapper {

template <class Iterator>
class rle_view
{
    public:
        using symbol_type = typename Iterator::value_type;
        using length_type = std::size_t;
        class rl_iterator
        {
            public:
                using iterator_category = std::forward_iterator_tag;
                using difference_type   = std::ptrdiff_t;
                using value_type        = length_type;
                using pointer           = value_type*;
                using reference         = value_type&;

                rl_iterator(rle_view const& view, Iterator iterator) 
                    : parent_view(view), itr(iterator), run_length(0)
                {
                    init();
                }

                value_type operator*() const noexcept {return run_length;}

                rl_iterator const& operator++()
                {
                    return advance();
                }

                rl_iterator operator++(int) 
                {
                    auto current = *this; 
                    ++itr; 
                    return current;
                }

                symbol_type get_symbol() const {return *itr;}

            private:
                rle_view const& parent_view;
                Iterator itr;
                length_type run_length;

                void init()
                {
                    run_length = 0;
                    if (itr != parent_view.itr_stop) {
                        auto cur_val = *itr;
                        decltype(itr) prev;
                        while (*itr == cur_val and itr != parent_view.itr_stop) {
                            prev = itr++;
                            ++run_length;
                        }
                        itr = prev;
                    }
                }

                rl_iterator const& advance() 
                {
                    run_length = 0;
                    if (itr != parent_view.itr_stop) ++itr;
                    if (itr != parent_view.itr_stop) {
                        auto cur_val = *itr;
                        decltype(itr) prev;
                        while (itr != parent_view.itr_stop and *itr == cur_val) {
                            prev = itr;
                            ++itr;
                            ++run_length;
                        }
                        itr = prev;
                        assert(run_length);
                    }
                    return *this;
                }

                friend bool operator==(rl_iterator const& a, rl_iterator const& b) 
                {
                    bool same_view = &a.parent_view == &b.parent_view;
                    bool same_position = a.itr == b.itr;
                    return same_view and same_position;
                }
                friend bool operator!=(rl_iterator const& a, rl_iterator const& b) { return not (a == b); }
        };

        class const_iterator
        {
            public:
                using iterator_category = std::forward_iterator_tag;
                using difference_type   = std::ptrdiff_t;
                using value_type        = std::pair<symbol_type, length_type>;
                using pointer           = value_type*;
                using reference         = value_type&;

                const_iterator(rle_view const& view, Iterator iterator) : itr(view, iterator) {}
                value_type operator*() const {return std::make_pair(itr.get_symbol(), *itr);}
                const_iterator const& operator++() {++itr; return *this;}
                const_iterator operator++(int) {auto current = *this; ++itr; return current;}

            private:
                rl_iterator itr;
                friend bool operator==(const_iterator const& a, const_iterator const& b) {return a.itr == b.itr;}
                friend bool operator!=(const_iterator const& a, const_iterator const& b) {return not (a == b);}
        };

        rle_view(Iterator start, Iterator stop) noexcept : itr_start(start), itr_stop(stop) {}
        const_iterator cbegin() const noexcept {return const_iterator(*this, itr_start);}
        const_iterator cend() const noexcept {return const_iterator(*this, itr_stop);}
        const_iterator begin() const {return cbegin();}
        const_iterator end() const noexcept {return cend();}
        rl_iterator rl_cbegin() const noexcept {return rl_iterator(*this, itr_start);}
        rl_iterator rl_cend() const noexcept {return rl_iterator(*this, itr_stop);}
    
    private:
        Iterator itr_start;
        Iterator itr_stop;
};

} // namespace wrapper

#endif // RLE_VIEW_HPP