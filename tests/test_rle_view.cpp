#include <random>
#include <vector>
#include <iostream>
#include <cassert>
#include "../include/rle_view.hpp"

std::vector<std::size_t> get_repeated_sequence(std::mt19937& gen, std::size_t size, std::size_t max_rep_len)
{
    std::vector<std::size_t> sequence;
    std::uniform_int_distribution<std::size_t> distrib(0, max_rep_len);
    std::size_t cur_val = 0;
    while (sequence.size() < size) {
        auto rep_len = distrib(gen);
        for (std::size_t i = 0; i < rep_len; ++i) sequence.push_back(cur_val);
        ++cur_val;
    }
    return sequence;
}

void test_simple_rle_view()
{
    std::vector<std::size_t> const& sequence = {0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,2,3,3,3,3,3,3,3,3,3,3,3};
    wrapper::rle_view view(sequence.begin(), sequence.end());
    for (auto elem : view) {
        std::cerr << "(" << elem.first << ", " << elem.second << "), ";
    }
    std::cerr << "\n";
}

void test_rle_view(std::vector<std::size_t> const& sequence)
{
    wrapper::rle_view view(sequence.begin(), sequence.end());
    for (auto elem : view) {
        std::cerr << "(" << elem.first << ", " << elem.second << "), ";
    }
    std::cerr << "\n";
}

int main()
{
    using namespace std;
    const size_t seed = 42;
    const size_t vector_size = 1000 * 1000;
    const size_t max_rep = 500;
    std::mt19937 gen(seed); // Standard mersenne_twister_engine seeded with rd()

    auto seq = get_repeated_sequence(gen, vector_size, max_rep);
    test_simple_rle_view();
    // test_rle_view(seq);

    std::cerr << "Everything is OK\n";
    return 0;
}