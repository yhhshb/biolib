#include <cmath>
#include <random>
#include <vector>
#include <iostream>
#include "../include/bit_vector.hpp"
#include "../include/rank_select.hpp"
#include "../include/rank_select_64.hpp"

#include "../include/constants.hpp"

template <typename T, std::size_t bbs, std::size_t sbbs>
void check_rs(size_t seed, size_t vector_size, size_t insertions);

int main()
{
    using namespace std;
    const size_t seed = 42;
    const size_t insertions = 1000;
    const size_t binary_vector_size = 100 * insertions;

    auto c = log2(binary_vector_size);
    auto num_super_blocks = 2 * binary_vector_size / (c*c);
    auto super_block_bit_size = c;
    auto num_blocks = 2 * binary_vector_size / c;
    auto block_bit_size = static_cast<size_t>(ceil(std::log2(c)));
    auto num_blocks_per_super_block = super_block_bit_size / block_bit_size;

    cerr << "number of super blocks = " << num_super_blocks << "\n";
    cerr << "super block bit size = " << super_block_bit_size << " bits\n";
    cerr << "number of blocks = " << num_blocks << "\n";
    cerr << "block bit size = " << block_bit_size << " bits\n";
    cerr << "number of blocks per super block = " << num_blocks_per_super_block << "\n";

    auto super_blocks_overhead = num_super_blocks * super_block_bit_size;
    auto blocks_overhead = num_blocks * block_bit_size;
    cerr << "expected overhead = " << super_blocks_overhead + blocks_overhead << " = ";
    cerr << "(spo + bo) = " << super_blocks_overhead << " + " << blocks_overhead << "\n";

    cerr << "Testing constexpr log2:\n";
    cerr << "log2(5)" << constants::log2(5) << "\n";
    cerr << "log2(4)" << constants::log2(5) << "\n";

    // check_rs<uint8_t, 4, 4>(seed, binary_vector_size, insertions);
    check_rs<uint16_t, 4, 4>(seed, binary_vector_size, insertions);
    check_rs<uint32_t, 4, 3>(seed, binary_vector_size, insertions);
    check_rs<uint32_t, 4, 3>(seed, binary_vector_size, insertions);
    check_rs<uint64_t, 64, 7>(seed, binary_vector_size, insertions);
    // check_rs<uint64_t, 64, 8>(seed, binary_vector_size, insertions); // this should be faster since specialised

    cerr << "Everything is OK\n";
    return 0;
}

template <typename T, std::size_t bbs, std::size_t sbbs>
void check_rs(size_t seed, size_t vector_size, size_t insertions)
{
    std::cerr << "----------------------------------------------------------------\n";
    std::mt19937 gen(seed); // Standard mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<std::size_t> distrib(0, vector_size - 1);
    bit::vector<T> bvec(vector_size, false);
    for (std::size_t i = 0; i < insertions; ++i) {
        std::size_t rp = distrib(gen);
        bvec.set(rp);
    }
    bvec.set(0);
    // std::cerr << "\nInsertions inside binary vector: Done\n";
    bit::rank_select<decltype(bvec), bbs, sbbs> rs_vec(std::move(bvec));
    std::size_t rank = 0;
    for (std::size_t i = 0; i < rs_vec.size(); ++i) {
        // std::cerr << "rank of " << i << " = ";
        auto rs_rank = rs_vec.rank1(i);
        // std::cerr << rs_rank << " (true rank = " << rank << ")\n";
        assert(rs_rank == rank);
        if (rs_vec.data().at(i)) ++rank;
    }
    // std::cerr << "rank/select size = " << rs_vec.bit_size() << "\n";
    std::cerr << "rank/select overhead = " << rs_vec.bit_overhead() << "\n";
    // std::cerr << "****************************************************************\n";
}