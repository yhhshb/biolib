#include <random>
#include <iostream>
#include <cassert>
#include "../include/bit_vector.hpp"

template <typename T>
void check_bit_vector(size_t seed, size_t vector_size, size_t insertions);

int main()
{
    using namespace std;

    const size_t seed = 42;
    const size_t insertions = 100000;
    const size_t binary_vector_size = 10 * insertions;
    
    check_bit_vector<uint8_t>(seed, binary_vector_size, insertions);
    check_bit_vector<uint16_t>(seed, binary_vector_size, insertions);
    check_bit_vector<uint32_t>(seed, binary_vector_size, insertions);
    check_bit_vector<uint64_t>(seed, binary_vector_size, insertions);
    cerr << "Everything is OK\n";

    return 0;
}

template <typename T>
void check_bit_vector(size_t seed, size_t vector_size, size_t insertions)
{
    std::mt19937 gen(seed); // Standard mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<std::size_t> distrib(0, vector_size - 1);
    bit::vector<T> bvec;
    assert(bvec.size() == 0);

    // Check resizing
    bvec.resize(vector_size, false);
    assert(bvec.size() == vector_size);
    { // Check if it is all false
        for (std::size_t i = 0; i < bvec.size(); ++i) { // by accessing blocks using the normal index
            assert(bvec.block_at(i) == static_cast<T>(0));
        }
        auto vec = bvec.vector_data();
        for (auto itr = vec.cbegin(); itr != vec.cend(); ++itr) { // or by iterating over the undelying blocks
            assert(*itr == static_cast<T>(0));
        }
    }

    for (std::size_t i = 0; i < insertions; ++i) { // check method set(idx)
        std::size_t rp = distrib(gen);
        // std::cerr << "setting position " << rp << ", vector size = " << bvec.size() << std::endl;
        bvec.set(rp);
    }

    { // Check unary increment of bit iterator
        std::size_t j = 0;
        for (auto itr = bvec.cbegin(); itr != bvec.cend(); ++itr) {
            auto v = *itr;
            assert(v == bvec.at(j));
            ++j;
        }
    }

    { // Check arbitrary increment of bit iterator
        for (std::size_t i = 0; i < bvec.size(); ++i) {
            auto v = *(bvec.cbegin() + i);
            auto a = bvec.at(i);
            // std::cerr << "i = " << i << ", v = " << v << ", a = " << a << "\n";
            assert(v == a);
        }
    }

    for (std::size_t i = 0; i < insertions; ++i) { // check method clear(idx)
        if (bvec.at(i)) {
            bvec.clear(i);
            assert(not bvec.at(i));
        }
    }
    bvec.clear();
    assert(bvec.size() == 0);
}