#include <random>
#include <vector>
#include "../include/bit_vector.hpp"
#include "../include/rank_select.hpp"
#include "../include/rank_select_64.hpp"

template <typename T, std::size_t bbs, std::size_t sbbs>
void check_rs(size_t seed, size_t vector_size, size_t insertions);

int main()
{
    using namespace std;
    const size_t seed = 42;
    const size_t insertions = 1000;
    const size_t binary_vector_size = 10 * insertions;
    
    check_rs<uint8_t, 9, 10>(seed, binary_vector_size, insertions);
    // check_rs<uint16_t, 15, 8>(seed, binary_vector_size, insertions);
    // check_rs<uint32_t, 8, 8>(seed, binary_vector_size, insertions);
    // check_rs<uint32_t, 32, 16>(seed, binary_vector_size, insertions);
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
        std::cerr << rp << ",";
        bvec.set(rp);
    }
    bvec.set(0);
    std::cerr << "\nInsertions inside binary vector: Done\n";
    bit::rank_select<decltype(bvec), bbs, sbbs> rs_vec(std::move(bvec));
    std::size_t rank = 0;
    for (std::size_t i = 0; i < rs_vec.size(); ++i) {
        auto rs_rank = rs_vec.rank1(i);
        std::cerr << "rank of " << i << " = " << rs_rank << " (true rank = " << rank << ")\n";
        assert(rs_rank == rank);
        if (rs_vec.data().at(i)) ++rank;
    }
    std::cerr << "****************************************************************\n";
}