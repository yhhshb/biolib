#ifndef SELECT_HINTS_HPP
#define SELECT_HINTS_HPP

#include <vector>

namespace bit{
namespace rs {

template <bool B>
class select_hints {};

template <>
class select_hints<false> 
{
    protected:
        std::size_t bit_size() const noexcept {return 0;}

        template <class Visitor>
        void visit([[maybe_unused]] Visitor& visitor) const {}

        template <class Visitor>
        void visit([[maybe_unused]] Visitor& visitor) {}
};

template <>
class select_hints<true> 
{
    protected:
        using hint_t = std::size_t;
        std::vector<hint_t> hints;

        std::size_t bit_size() const noexcept {return hints.size() * sizeof(hint_t) * 8;}

        template <class Visitor>
        void visit(Visitor& visitor) const {visitor.visit(hints);}

        template <class Visitor>
        void visit(Visitor& visitor) {visitor.visit(hints);}
};

}
}

#endif