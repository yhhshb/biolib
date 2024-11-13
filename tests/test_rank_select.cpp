#include <cmath>
#include <random>
#include <vector>
#include <iostream>
#include <algorithm>

#include "../include/bit_vector.hpp"
#include "../include/rank_select.hpp"
#include "../include/io.hpp"

#include "../include/constants.hpp"

template <typename T, std::size_t, std::size_t, bool, bool>
void check_rs(size_t, size_t, size_t);
int test_for(size_t insertions, size_t multiplier, size_t seed);

int main()
{
    int err = 0;
    if(!err) err = test_for(1000, 2, 42); //dense
    if(!err) err = test_for(1000, 5, 42);
    if(!err) err = test_for(1000, 10, 42);
    if(!err) err = test_for(1000, 100, 42);
    if(!err) err = test_for(1000, 1000, 42); // very sparse
    return err;
}

int test_for(size_t insertions, size_t multiplier, size_t seed)
{
    size_t binary_vector_size = insertions * multiplier;
    const auto c = log2(binary_vector_size);
    const auto super_block_spanning_bit_size = (c*c)/2;
    const auto num_super_blocks = binary_vector_size / super_block_spanning_bit_size;
    const auto block_spanning_bit_size = c/2;
    const auto num_blocks = binary_vector_size / block_spanning_bit_size;
     
    const auto num_blocks_per_super_block = super_block_spanning_bit_size / block_spanning_bit_size;

    std::cerr << "From theory:\n";
    std::cerr << "\tnumber of super blocks = " << num_super_blocks << "\n";
    std::cerr << "\teach super block spans " << super_block_spanning_bit_size << " bits\n";
    std::cerr << "\tnumber of blocks = " << num_blocks << "\n";
    std::cerr << "\teach block spans " << block_spanning_bit_size << " bits" << "(each counter is should use " << std::log2(c) << " bits)\n";
    std::cerr << "\tnumber of blocks per super block = " << num_blocks_per_super_block << "\n";

    const auto super_blocks_overhead = num_super_blocks * c;
    const auto blocks_overhead = num_blocks * std::log2(c);
    std::cerr << "\texpected overhead = " << super_blocks_overhead + blocks_overhead << " = ";
    std::cerr << "\t(spo + bo) = " << super_blocks_overhead << " + " << blocks_overhead << "\n";

    constexpr std::size_t block_span_bit_size = 14;
    constexpr std::size_t super_block_block_size = 27;

    check_rs<uint8_t, block_span_bit_size, super_block_block_size, true, true>(seed, binary_vector_size, insertions);
    check_rs<uint16_t, block_span_bit_size, super_block_block_size, false, false>(seed, binary_vector_size, insertions);
    check_rs<uint32_t, block_span_bit_size, super_block_block_size, false, false>(seed, binary_vector_size, insertions);
    check_rs<uint64_t, block_span_bit_size, super_block_block_size, false, false>(seed, binary_vector_size, insertions);
    check_rs<uint64_t, 64, 8, false, false>(seed, binary_vector_size, insertions); // this should be faster since specialised
    check_rs<uint64_t, 64, 8, true, true>(seed, binary_vector_size, insertions); // this should be faster since specialised

    std::cerr << "Everything is OK\n";
    return 0;
}

template <typename T, std::size_t bbs, std::size_t sbbs, bool with_select1_hints, bool with_select0_hints>
void check_rs(size_t seed, size_t vector_size, size_t insertions)
{
    std::cerr << "----------------------------------------------------------------\n";
    std::mt19937 gen(seed); // Standard mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<std::size_t> distrib(0, vector_size - 1);
    std::vector<std::size_t> inserted;
    bit::vector<T> bvec(vector_size, false);
    for (std::size_t i = 0; i < insertions; ++i) {
        std::size_t rp = distrib(gen);
        bvec.set(rp);
        inserted.push_back(rp);
    }
    bvec.set(0);
    inserted.push_back(0);
    bvec.set(bvec.size() - 1); // check size1 later
    inserted.push_back(bvec.size() - 1);
    std::sort(inserted.begin(), inserted.end());
    inserted.erase(std::unique(inserted.begin(), inserted.end() ), inserted.end());

    // for (auto bblock : bvec.vector_data()) {
    //     std::cerr << std::size_t(bblock) << ", ";
    // }; 
    // std::cerr << "\n";

    // for (auto itr = bvec.cbegin(); itr != bvec.cend(); ++itr) {
    //     std::cerr << *itr;
    // }
    // std::cerr << "\n";

    bit::rs::array<decltype(bvec), bbs, sbbs, with_select1_hints, with_select0_hints> dummy(std::move(bvec));
    bvec.clear();
    bit::rs::array<decltype(bvec), bbs, sbbs, with_select1_hints, with_select0_hints> rs_vec(std::move(bvec));
    {
        std::string sname = "tmp.bin";
        // auto copy = rs_vec;
        io::store(dummy, sname);
        rs_vec = io::load<decltype(rs_vec)>(sname);
        assert(dummy == rs_vec);
    }

    // bit::rs::array<decltype(bvec), bbs, sbbs, with_select1_hints, with_select0_hints> rs_vec(std::move(bvec));

    if (rs_vec.size1() != inserted.size()) {
        // std::cerr << rs_vec.size1() << " != " << inserted.size() << "\n";
        throw std::runtime_error("[size1] FAIL");
    }

    std::size_t rank = 0;
    for (std::size_t i = 0; i < rs_vec.size(); ++i) {
        auto rs_rank = rs_vec.rank1(i);
        // std::cerr << "rank of " << i << " = ";
        // std::cerr << rs_rank << " (true rank = " << rank << ")\n";
        if (rs_rank != rank) throw std::runtime_error("[rank1] FAIL");
        if (rs_vec.data().at(i)) {
            // std::cerr << "bit vector[" << i << "] is set\n"; 
            ++rank;
        }
    }
    // std::cerr << "Rank check DONE\n";

    for (std::size_t i = 0; i < inserted.size(); ++i) {
        auto idx = rs_vec.select1(i);
        // std::cerr << "select [" << i << "] = " << idx << "(true answer = " << inserted.at(i) << ")\n";
        if (idx != inserted.at(i)) throw std::runtime_error("[select1] FAIL");
    }

    {
        auto missing = vector_size - inserted.size();
        for (std::size_t i = 0; i < missing; ++i) {
            auto idx = rs_vec.select0(i);
            if (idx > vector_size) throw std::out_of_range("[select0] retrieved index out of range");
            inserted.push_back(idx);
        }
        if (inserted.size() != vector_size) throw std::runtime_error("[select0] size mismatch");
        std::sort(inserted.begin(), inserted.end());
        for (std::size_t i = 0; i < inserted.size(); ++i) {
            if (i != inserted.at(i)) throw std::runtime_error("[select0] duplicated elements");
        }
    }

    std::cerr << "rank/select size = " << rs_vec.bit_size() << "\n";
    std::cerr << "rank/select overhead = " << rs_vec.bit_overhead() << "\n";
    std::cerr << "****************************************************************\n";
}