#include <type_traits>
#include <cstdint>
#include <array>
#include <cassert>
#include <iostream>


#include "../include/hash.hpp"

int main()
{
    std::cout << std::boolalpha;
    std::cout << std::is_fundamental<uint8_t>::value << "\n";
    std::cout << std::is_fundamental<const uint8_t>::value << "\n";
    std::cout << std::endl;

    std::array<uint8_t, 2> arr = {0, 1};
    hash::double_hash64 hasher;
    assert(hasher(arr.data(), arr.size(), 0) != hasher(&arr[1], arr.size()-1, 0));
}