#include <cmath>
#include <random>
#include <iostream>
#include <cassert>
#include "../include/bit_operations.hpp"

template <typename T>
constexpr int emulated_parity(T x) // local version of generic parity computation
{
    bool parity = false;  // parity will be the parity of v

    while (x) {
        parity = !parity;
        x = x & (x - 1);
    }
    return static_cast<int>(parity);
}

void test_number(std::size_t t)
{
    auto least = bit::lsb(t);
    auto most = bit::msb(t);
    if (t) {
        assert(least);
        assert(most);
        // std::cerr << "x = " << t << ", lsb = " << least.value() << ", msb = " << most.value() << "\n";
        assert(most = static_cast<std::size_t>(std::floor(std::log2(t))));
        std::size_t lsb_check = 0;
        while(not (t & 1ULL)) {
            t >>= 1;
            ++lsb_check;
        }
        assert(least.value() == lsb_check);
    } else {
        assert(not least);
        assert(not most);
    }
    assert(bit::parity(t) == emulated_parity(t));
}

int main()
{
    std::size_t x = 1;
    // assert((x << (sizeof(x) * 8)) == 0);
    for(std::size_t i = 0; i <= 64; ++i) {
        auto t = i < 8 * sizeof(x) ? (x << i) : 0; 
        test_number(t);
    }

    std::mt19937 gen(42);
    std::uniform_int_distribution<std::size_t> distrib(0, std::numeric_limits<std::size_t>::max());
    for (std::size_t i = 0; i < 1000000; ++i) {
        auto x = distrib(gen);
        test_number(x);
    }

    std::cerr << "Everything is OK\n";

    return 0;
}