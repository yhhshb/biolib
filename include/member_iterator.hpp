#include <type_traits>

namespace iterators {

template <class Iterator, typename Function>
class member_iterator 
{
    public:
        using iterator_category = typename Iterator::iterator_category;
        using difference_type   = typename Iterator::difference_type;
        using value_type        = typename std::invoke_result<Function, typename Iterator::value_type>::type;
        using pointer           = typename Iterator::pointer;
        using reference         = typename Iterator::reference;

        member_iterator(Iterator itr, Function member_access) 
            : hidden(itr), access(member_access)
        {}

        value_type operator*() 
        {
            return access(*hidden);
        }

        member_iterator const& operator++() noexcept
        {
            ++hidden;
            return *this;
        }

        member_iterator operator++(int) noexcept
        {
            auto current = *this;
            operator++();
            return current;
        }

    private:
        Iterator hidden;
        Function access;

        friend bool operator==(member_iterator const& a, member_iterator const& b) 
        {
            return a.hidden == b.hidden;
        }
        friend bool operator!=(member_iterator const& a, member_iterator const& b) { return not (a == b); }
};

} // namespace iterators