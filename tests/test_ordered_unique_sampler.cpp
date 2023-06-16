/*
 * Test Ordered Unique Sampler
 */

#include <random>
#include <string>
#include <cassert>
#include <argparse/argparse.hpp>
#include "../include/ordered_unique_sampler.hpp"

int main()
{
    const std::size_t N = 10000;
    const std::size_t m = 0;
    const std::size_t M = 9;

    std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<std::size_t> distrib(m, M);

    assert((M-m) <= N); // the must contain at least one copy of numbers in between m and M

    std::vector<std::size_t> v;
    v.reserve(N);
    for (std::size_t i = m; i < M; ++i) v.push_back(i);
    while(v.size() < N) v.push_back(distrib(gen));

    assert(v.size() == N);
    std::sort(v.begin(), v.end());
    
    sampler::ordered_unique_sampler s(v.cbegin(), v.cend());
    std::vector<std::size_t> unique(s.cbegin(), s.cend());
    auto itr = unique.begin();
    for (std::size_t i = m; i < M; ++i) {
        if (itr == unique.end() or i != *itr) {
            std::cerr << "FAILURE : ordered sampler" << std::endl;
            return 1;
        }
        ++itr;
    }

    std::cerr << "PASS : ordered sampler" << std::endl;

    return 0;
}