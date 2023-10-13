#include <cmath>
#include <random>
#include <vector>
#include <iostream>

#include "../include/elias_fano.hpp"
#include "../include/cumulative_iterator.hpp"
#include "../include/io.hpp"

int main()
{
    using namespace std;
    using namespace iterators;
    const size_t seed = 42;
    const size_t vector_size = 1000 * 1000;
    const size_t max_delta = 500;
    std::mt19937 gen(seed); // Standard mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<std::size_t> distrib(0, max_delta);
    std::vector<std::size_t> sequence;
    std::vector<std::size_t> cumulative_sequence;
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
            cumulative_sequence.push_back(*itr);
            // std::cerr << "i = " << k << ", seq[" << k << "] = " << sequence.at(k) << ", cumulative sum = " << *itr << "\n";
        }
    }
    std::cerr << "Cumulative iterator OK\n";
    bit::ef::array ef_sequence(cumulative_iterator(sequence.begin()), cumulative_iterator(sequence.end()));
    assert(ef_sequence.size() == sequence.size());
    for (std::size_t i = 0; i < ef_sequence.size(); ++i) {
        // std::cerr << "ef size = " << ef_sequence.size() << "\n";
        auto diff_ef = ef_sequence.diff_at(i);
        // std::cerr << "retrieved diff = " << diff_ef << ", true value = " << sequence.at(i) << "\n";
        if (diff_ef != sequence.at(i)) throw runtime_error("FAIL");
    }
    {
        std::size_t i = 0;
        auto tr = ef_sequence.cbegin();
        for (auto itr = cumulative_sequence.begin(); itr != cumulative_sequence.end(); ++itr) {
            if (*tr != *itr) throw runtime_error("FAIL");
            ++tr;
        }
        std::cerr << "const iterator OK\n";
        std::mt19937 gen(seed); // Standard mersenne_twister_engine seeded with rd()
        std::uniform_int_distribution<std::size_t> distrib(0, ef_sequence.size() - 1);
        i = distrib(gen);
        std::size_t j = distrib(gen);
        if (j < i) {auto k = j; j = i; i = k;}
        auto jend = bit::ef::array::const_iterator(ef_sequence, j);
        for (auto itr = bit::ef::array::const_iterator(ef_sequence, i); itr != jend; ++itr) {
            if (*itr != cumulative_sequence.at(i)) throw runtime_error("FAIL");
            ++i;
        }
        std::cerr << "const iterator starting at random positions OK\n";
    }

    {
        leq_find
        geq_find
    }

    { // Check writing and reading
        std::string sname = "tmp.bin";
        auto copy = ef_sequence;
        io::store(ef_sequence, sname);
        ef_sequence = io::load<decltype(ef_sequence)>(sname);
        assert(copy == ef_sequence);
    }

    std::cerr << "Everything is OK\n";
    return 0;
}