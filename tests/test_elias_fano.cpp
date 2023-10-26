#include <cmath>
#include <random>
#include <vector>
#include <iostream>

#include "../include/elias_fano.hpp"
#include "../include/cumulative_iterator.hpp"
#include "../include/io.hpp"

std::vector<std::size_t> get_random_sequence(std::mt19937& gen, std::size_t size, std::size_t delta);
std::vector<std::size_t> get_cumulative_sequence(std::vector<std::size_t> const& sequence);
bit::ef::array get_ef_sequence(std::vector<std::size_t> const& sequence, std::vector<std::size_t> const& cumulative_sequence);
void test_const_iterator_forward_access(bit::ef::array const& ef_sequence, std::vector<std::size_t> const& cumulative_sequence);
void test_const_iterator_reverse_access(bit::ef::array const& ef_sequence, std::vector<std::size_t> const& cumulative_sequence);
void test_const_iterator_random_access(std::mt19937& gen, bit::ef::array const& ef_sequence, std::vector<std::size_t> const& cumulative_sequence);
void test_rw(bit::ef::array const& ef_sequence);
void test_lg_find(std::mt19937& gen, bit::ef::array const& ef_sequence, std::vector<std::size_t> const& cumulative_sequence);
void test_lg_find_simple_example();

int main()
{
    using namespace std;
    
    const size_t seed = 42;
    const size_t vector_size = 1000 * 1000;
    const size_t max_delta = 500;
    std::mt19937 gen(seed); // Standard mersenne_twister_engine seeded with rd()

    auto seq = get_random_sequence(gen, vector_size, max_delta);
    auto cseq = get_cumulative_sequence(seq);
    auto efseq = get_ef_sequence(seq, cseq);
    test_rw(efseq);
    test_const_iterator_forward_access(efseq, cseq);
    test_const_iterator_reverse_access(efseq, cseq);
    test_const_iterator_random_access(gen, efseq, cseq);
    test_lg_find(gen, efseq, cseq);
    test_lg_find_simple_example();

    std::cerr << "Everything is OK\n";
    return 0;
}

std::vector<std::size_t> get_random_sequence(std::mt19937& gen, std::size_t size, std::size_t delta)
{
    std::vector<std::size_t> sequence;
    std::uniform_int_distribution<std::size_t> distrib(0, delta);
    for (std::size_t i = 0; i < size; ++i) {
        sequence.push_back(distrib(gen));
    }
    return sequence;
}

std::vector<std::size_t> get_cumulative_sequence(std::vector<std::size_t> const& sequence)
{
    using namespace iterators;
    std::vector<std::size_t> cumulative_sequence;
    std::size_t k = 0;
    std::size_t sum = 0;
    for (auto itr = cumulative_iterator(sequence.begin()); itr != cumulative_iterator(sequence.end()); ++itr, ++k)
    {
        sum += sequence.at(k);
        if (sum != *itr) throw std::runtime_error("[cumulative_iterator] FAIL (value)");
        cumulative_sequence.push_back(*itr);
    }
    if (cumulative_sequence.size() != sequence.size()) throw std::runtime_error("[cumulative_iterator] FAIL (size)");
    std::cerr << "Cumulative iterator OK\n";
    return cumulative_sequence;
}

bit::ef::array get_ef_sequence(std::vector<std::size_t> const& sequence, std::vector<std::size_t> const& cumulative_sequence)
{
    using namespace iterators;
    bit::ef::array ef_sequence(cumulative_iterator(sequence.begin()), cumulative_iterator(sequence.end()));
    assert(ef_sequence.size() == sequence.size());
    for (std::size_t i = 0; i < ef_sequence.size(); ++i) {
        auto diff_ef = ef_sequence.diff_at(i);
        auto at_ef = ef_sequence.at(i);
        if (diff_ef != sequence.at(i)) throw std::runtime_error("[diff_at] FAIL");
        // std::cerr << at_ef << " == " << cumulative_sequence.at(i) << "\n";
        // std::cerr << ef_sequence.at(1) << "\n";
        if (at_ef != cumulative_sequence.at(i)) throw std::runtime_error("[at] FAIL");
    }
    std::cerr << "Elias-Fano sequence OK\n";
    return ef_sequence;
}

void test_rw(bit::ef::array const& ef_sequence)
{
    std::string sname = "tmp.bin";
    bit::ef::array copy = ef_sequence;
    io::store(copy, sname);
    copy = io::load<decltype(copy)>(sname);
    assert(copy == ef_sequence);
    std::cerr << "Writing/Reading OK\n";
}

void test_const_iterator_forward_access(bit::ef::array const& ef_sequence, std::vector<std::size_t> const& cumulative_sequence)
{
    auto tr = ef_sequence.cbegin();
    for (auto itr = cumulative_sequence.begin(); itr != cumulative_sequence.end(); ++itr) {
        if (*tr != *itr) throw std::runtime_error("[iterator (full pass)] FAIL");
        ++tr;
    }
    std::cerr << "const iterator operator++ OK\n";
}

void test_const_iterator_reverse_access(bit::ef::array const& ef_sequence, std::vector<std::size_t> const& cumulative_sequence)
{
    auto tr = bit::ef::array::const_iterator(ef_sequence, ef_sequence.size() - 1);
    for (auto itr = cumulative_sequence.rbegin(); itr != cumulative_sequence.rend(); ++itr) {
        // std::cerr << *tr << " == " << *itr << "\n";
        // std::cerr << "tr index = " << tr - ef_sequence.cbegin() << "\n";
        if (*tr != *itr) throw std::runtime_error("[iterator (full reverse pass)] FAIL");
        --tr;
    }
    // auto tr2 = ef_sequence.cend();
    // --tr2;
    // assert(*tr2 == ef_sequence.back());
    std::cerr << "const iterator operator-- OK\n";
}

void test_const_iterator_random_access(std::mt19937& gen, bit::ef::array const& ef_sequence, std::vector<std::size_t> const& cumulative_sequence)
{
    assert(ef_sequence.size() == cumulative_sequence.size());
    std::size_t i = 0;
    for (i = 0; i < ef_sequence.size(); ++i) {
        auto itr = bit::ef::array::const_iterator(ef_sequence, i);
        // std::cerr << "i = " << i << ", *itr = " << *itr << ", true value = " << cumulative_sequence.at(i) << ", elias fano value = " << ef_sequence.at(i) << "\n";
        if (*itr != cumulative_sequence.at(i) or *itr != ef_sequence.at(i)) throw std::runtime_error("[iterator (random position)] FAIL (value check)");
    }
    std::uniform_int_distribution<std::size_t> distrib(0, ef_sequence.size() - 1);
    i = distrib(gen);
    std::size_t j = distrib(gen);
    if (j < i) {auto k = j; j = i; i = k;}
    auto jend = bit::ef::array::const_iterator(ef_sequence, j);
    assert(jend - ef_sequence.cbegin() == j);
    std::cerr << "trying to query subsequence\n";
    for (auto itr = bit::ef::array::const_iterator(ef_sequence, i); itr != jend; ++itr) {
        if (*itr != cumulative_sequence.at(i)) throw std::runtime_error("[iterator (random position)] FAIL (span check)");
        ++i;
    }
    std::cerr << "const iterator starting at random positions OK\n";
}

void test_lg_find(std::mt19937& gen, bit::ef::array const& ef_sequence, std::vector<std::size_t> const& cumulative_sequence)
{
    std::uniform_int_distribution<std::size_t> distrib(0, cumulative_sequence.back());
    for (std::size_t dummy = 0; dummy < 10; ++dummy) { // try 10 different values
        auto x = distrib(gen);
        auto idx_leq = ef_sequence.lt_find(x);
        auto idx_geq = ef_sequence.gt_find(x);

        std::size_t leq_check = -1;
        for (std::size_t i = 0; i < cumulative_sequence.size() and cumulative_sequence.at(i) <= x; ++i) {
            leq_check = i;
        }
        // std::cerr << "x = " << x << "\n";
        // std::cerr << "leq_check = " << leq_check 
        //           << ", cumulative_seq[leq_check] = " << cumulative_sequence.at(leq_check) 
        //           << ", cumulative_seq[leq_check + 1] = " << (leq_check >= cumulative_sequence.size() ? -1 : cumulative_sequence.at(leq_check + 1)) 
        //           << ", ef[leq_check] = " << ef_sequence.at(leq_check)
        //           << "\n";
        // std::cerr << "idx (leq) = " << idx_leq 
        //           << ", cumulative_seq[idx] = " << cumulative_sequence.at(idx_leq)
        //           << ", cumulative_seq[idx + 1] = " << (idx_leq >= cumulative_sequence.size() ? -1 : cumulative_sequence.at(idx_leq + 1)) 
        //           << ", ef[idx] = " << ef_sequence.at(idx_leq)
        //           << "\n";
        assert(leq_check == std::size_t(-1) or cumulative_sequence.at(leq_check) < x);
        if (leq_check != idx_leq) throw std::runtime_error("[leq] FAIL");

        std::size_t geq_check = cumulative_sequence.size();
        for (std::size_t i = 0; i < cumulative_sequence.size() and cumulative_sequence.at(i) <= x; ++i) {
            geq_check = i;
        }
        ++geq_check;
        // std::cerr << "geq check = " << geq_check 
        //           << ", cumulative_seq[geq_check] = " << cumulative_sequence.at(geq_check)
        //           << ", cumulative_seq[geq_check - 1] = " << (geq_check == 0 ? -1 : cumulative_sequence.at(geq_check - 1)) 
        //           << "\n";
        // std::cerr << "idx (geq) = " << idx_geq << "\n";
        assert(geq_check == cumulative_sequence.size() or cumulative_sequence.at(geq_check) >= x);
        if (geq_check != idx_geq) throw std::runtime_error("[geq] FAIL");
    }
    std::cerr << "lt and gt OK\n";
}

void test_lg_find_simple_example()
{
    using namespace iterators;
    std::vector<std::size_t> repetitive;
    bit::ef::array rep_ef_seq;
    std::size_t idx;
    std::size_t cumulative_query;

    repetitive = {2,0,0,0,0,0,0,4,0,0,0,0,7,0,0,0,0,0};
    rep_ef_seq = bit::ef::array(cumulative_iterator(repetitive.begin()), cumulative_iterator(repetitive.end()));
    cumulative_query = 6;
    idx = rep_ef_seq.lt_find(cumulative_query);
    assert(idx == 6);
    idx = rep_ef_seq.gt_find(cumulative_query);
    assert(idx == 12);
    idx = rep_ef_seq.lt_find(cumulative_query, true);
    assert(idx == 0);
    idx = rep_ef_seq.gt_find(cumulative_query, true);
    assert(idx == repetitive.size() -1);

    repetitive = {2,0,2,2}; // u = 6, n = 4, u/n = 1 -> log2(u/n) = 0 -> shift = 0
    rep_ef_seq = bit::ef::array(cumulative_iterator(repetitive.begin()), cumulative_iterator(repetitive.end()));
    cumulative_query = 5; // try to find something > n but still < u
    idx = rep_ef_seq.lt_find(cumulative_query);
    assert(idx == 2);
    idx = rep_ef_seq.gt_find(cumulative_query);
    assert(idx == 3);
    idx = rep_ef_seq.lt_find(cumulative_query, true);
    assert(idx == 2);
    idx = rep_ef_seq.gt_find(cumulative_query, true);
    assert(idx == 3);

    std::cerr << "Simple lt and gt check OK\n";
}