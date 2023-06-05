#include <cmath>
#include <random>
#include <vector>
#include <iostream>

#include "../include/elias_fano.hpp"
#include "../include/cumulative_iterator.hpp"

int main()
{
    using namespace std;
    using namespace iterators;
    const size_t seed = 42;
    const size_t vector_size = 10;//1000 * 1000;
    const size_t max_delta = 500;
    std::mt19937 gen(seed); // Standard mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<std::size_t> distrib(0, max_delta);
    std::vector<std::size_t> sequence;
    for (std::size_t i = 0; i < vector_size; ++i) {
        sequence.push_back(distrib(gen));
    }

    {
        std::size_t k = 0;
        std::size_t sum = 0;
        for (auto itr = cumulative_iterator(sequence.begin()); itr != cumulative_iterator(sequence.end()); ++itr, ++k)
        {
            sum += sequence.at(k);
            if (sum != *itr) throw runtime_error("[cumulative_iterator] FAIL");
            std::cerr << "i = " << k << ", seq[" << k << "] = " << sequence.at(k) << ", cumulative sum = " << *itr << "\n";
        }
    }
    std::cerr << "Cumulative iterator OK\n";
    bit::ef::array ef_sequence(cumulative_iterator(sequence.begin()), cumulative_iterator(sequence.end()));
    assert(ef_sequence.size() == sequence.size());
    for (std::size_t i = 0; i < ef_sequence.size(); ++i) {
        std::cerr << "bla\n";
        if (ef_sequence.diff_at(i) != sequence.at(i)) throw runtime_error("FAIL");
    }

    return 0;
}