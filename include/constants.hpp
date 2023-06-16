#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include <array>
#include <type_traits>

namespace constants {

// using namespace gcem;

const std::array<uint8_t, 256> seq_nt4_table = {
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 0, 4, 1, 4, 4, 4, 2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 0, 4, 1, 4, 4, 4, 2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4
};

const std::array<char, 4> bases = {'A', 'C', 'G', 'T'};

//-----------------------------------------------------------------------------

/*
 * Usage: decltype(ret(f)) where f is a function to extract return type of f.
 * Starting from C++17 there is std::invoke_result_t<f>
 */ 

template<typename R, typename... A>
R ret(R(*)(A...));

template<typename C, typename R, typename... A>
R ret(R(C::*)(A...));

//------------------------------------------------------------------------------

/*
 * Usage: 
        struct Example {
            int Foo;
            void Bar() {}
            std::string toString() { return "Hello from Example::toString()!"; }
        };

        struct Example2 {
            int X;
        };

        template<class T>
        std::string optionalToString(T* obj)
        {
            if constexpr(has_member(T, toString()))
                return obj->toString();
            else
                return "toString not defined";
        }

        int main() {
            static_assert(has_member(Example, Foo), 
                            "Example class must have Foo member");
            static_assert(has_member(Example, Bar()), 
                            "Example class must have Bar() member function");
            static_assert(!has_member(Example, ZFoo), 
                            "Example class must not have ZFoo member.");
            static_assert(!has_member(Example, ZBar()), 
                            "Example class must not have ZBar() member function");

            Example e1;
            Example2 e2;

            std::cout << "e1: " << optionalToString(&e1) << "\n";
            std::cout << "e1: " << optionalToString(&e2) << "\n";
        }
 */

template <typename T, typename F> constexpr auto has_member_impl(F&& f) -> decltype(f(std::declval<T>()), true) {return true;}
template <typename> constexpr bool has_member_impl(...) { return false; }
#define has_member(T, EXPR) has_member_impl<T>( [](auto&& obj)->decltype(obj.EXPR){} )

} // namespace constants

#endif // CONSTANTS_HPP
