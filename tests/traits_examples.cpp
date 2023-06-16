#include <type_traits>
#include <cstdint>
#include <array>
#include <cassert>
#include <iostream>
#include <vector>
#include "../include/hash.hpp"
#include "../include/constants.hpp"

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
    if constexpr (constants::has_member(T, toString()))
        return obj->toString();
    else
        return "toString not defined";
}

int main()
{
    std::cout << std::boolalpha;
    std::cout << std::is_fundamental<uint8_t>::value << "\n";
    std::cout << std::is_fundamental<const uint8_t>::value << "\n";
    std::cout << std::endl;

    std::array<uint8_t, 2> arr = {0, 1};
    hash::double_hash64 hasher;
    assert(hasher(arr.data(), arr.size(), 0) != hasher(&arr[1], arr.size()-1, 0));

    static_assert(constants::has_member(Example, Foo), 
                    "Example class must have Foo member");
    static_assert(constants::has_member(Example, Bar()), 
                    "Example class must have Bar() member function");
    static_assert(!constants::has_member(Example, ZFoo), 
                    "Example class must not have ZFoo member.");
    static_assert(!constants::has_member(Example, ZBar()), 
                    "Example class must not have ZBar() member function");

    Example e1;
    Example2 e2;

    std::cout << "e1: " << optionalToString(&e1) << "\n";
    std::cout << "e1: " << optionalToString(&e2) << "\n";
    return 0;
}