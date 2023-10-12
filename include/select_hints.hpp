#ifndef SELECT_HINTS_HPP
#define SELECT_HINTS_HPP

#include <vector>

namespace bit{
namespace rs {

template <bool B>
class select1_hints {};

template <>
class select1_hints<false> 
{
    protected:
        std::size_t bit_size() const noexcept {return 0;}

        template <class Visitor>
        void visit([[maybe_unused]] Visitor& visitor) const {}

        template <class Visitor>
        void visit([[maybe_unused]] Visitor& visitor) {}
};

template <>
class select1_hints<true> 
{
    protected:
        using hint_t = std::size_t;
        std::vector<hint_t> hints1;

        std::size_t bit_size() const noexcept {return hints1.size() * sizeof(hint_t) * 8;}

        template <class Visitor>
        void visit(Visitor& visitor) const {visitor.visit(hints1);}

        template <class Visitor>
        void visit(Visitor& visitor) {visitor.visit(hints1);}
};

template <bool B>
class select0_hints {};

template <>
class select0_hints<false> 
{
    protected:
        std::size_t bit_size() const noexcept {return 0;}

        template <class Visitor>
        void visit([[maybe_unused]] Visitor& visitor) const {}

        template <class Visitor>
        void visit([[maybe_unused]] Visitor& visitor) {}
};

template <>
class select0_hints<true> 
{
    protected:
        using hint_t = std::size_t;
        std::vector<hint_t> hints0;

        std::size_t bit_size() const noexcept {return hints0.size() * sizeof(hint_t) * 8;}

        template <class Visitor>
        void visit(Visitor& visitor) const {visitor.visit(hints0);}

        template <class Visitor>
        void visit(Visitor& visitor) {visitor.visit(hints0);}
};

}
}

#endif