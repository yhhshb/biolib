#include <random>
#include <iostream>
#include <cassert>
#include "../include/bit_operations.hpp"
#include "../include/bit_vector.hpp"

template <typename T>
void check_vector_popcount(size_t seed, size_t vector_size, size_t insertions);

int main()
{
    using namespace std;
    const size_t seed = 42;
    const size_t binary_vector_size = 1000000;
    const size_t insertions = 100000;
    // for (uint8_t i = 0; i < 255; ++i)cerr << "popcount " << uint16_t(i) << " = " << bit::popcount(i) << "\n";
    check_vector_popcount<uint8_t>(seed, binary_vector_size, insertions);
    check_vector_popcount<uint16_t>(seed, binary_vector_size, insertions);
    check_vector_popcount<uint32_t>(seed, binary_vector_size, insertions);
    check_vector_popcount<uint64_t>(seed, binary_vector_size, insertions);
    cerr << "Everything is OK\n";
    return 0;
}

template <typename T>
void check_vector_popcount(size_t seed, size_t vector_size, size_t insertions)
{
    std::mt19937 gen(seed); // Standard mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<std::size_t> distrib(0, vector_size - 1);
    bit::vector<T> bvec(vector_size, false);
    for (size_t i = 0; i < insertions; ++i) {
        std::size_t rp = distrib(gen);
        // std::cerr << "setting position " << rp << ", vector size = " << bvec.size() << std::endl;
        bvec.set(rp);
    }
    uint8_t const * const plain_vec_view = reinterpret_cast<uint8_t const * const>(bvec.data());
    std::vector<T> const& vec_vec_view = bvec.vector_data();

    // std::cerr << "nblocks = " << vec_vec_view.size() << "\n";
    auto popvecvec = bit::popcount(vec_vec_view, vec_vec_view.size());
    auto popplainvec = bit::rank1(plain_vec_view, vec_vec_view.size() * sizeof(T) * 8 - 1);
    // std::cerr << "whole popcount of vector view = " << popvecvec << "\n";
    // std::cerr << "whole popcount of array view = " << popplainvec << std::endl;
    assert(popvecvec == popplainvec);

    for (size_t i = 0; i < vec_vec_view.size(); ++i) {
        popvecvec = bit::popcount(vec_vec_view, i + 1);
        // std::cerr << "block [" << i << "]\n";
        popplainvec = bit::rank1(plain_vec_view, (i + 1) * sizeof(T) * 8 - 1);
        // std::cerr << "popcount of vector view = " << popvecvec << "\n";
        // std::cerr << "popcount of array view = " << popplainvec << std::endl;
        assert(popvecvec == popplainvec);
    }
}