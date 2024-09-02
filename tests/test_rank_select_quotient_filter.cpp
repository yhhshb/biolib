#include <random>
#include <iostream>
#include <cassert>
#include <unordered_set>
#include <argparse/argparse.hpp>
#include "../include/toolbox.hpp"
#include "../include/rank_select_quotient_filter.hpp"

template <class QF>
int test_insertions_and_queries(QF& qf, std::size_t insertions, std::mt19937& generator)
{
    std::uniform_int_distribution<std::size_t> distribution(0, (1ULL << qf.hash_bitwidth()) - 1);
    std::unordered_set<uint64_t> check;
    std::cerr << ">>> Insertion\n";
    for (std::size_t i = 0; i < insertions; ++i) {
        auto x = distribution(generator);
        check.insert(x);
        qf.insert(x);
        // assert(qf.contains(x));
    }
    std::cerr << ">>> Query\n";
    for (auto x : check) {
        assert(qf.contains(x));
    }
    std::cerr << ">>> Iterator\n";
    for (auto itr = qf.cbegin(); itr != qf.cend(); ++itr) {
        assert(qf.contains(*itr));
    }
    std::cerr << ">>> Deletion\n";
    for (auto x : check) {
        qf.erase(x);
    }
    assert(qf.size() == 0);
    return 0;
}

template <class QF>
int test_user_parameters(QF& qf, std::size_t insertions, std::size_t deletions)
{
    // TODO
}

template <typename UnderlyingType>
int test_qf(uint64_t seed, uint8_t hash_bit_size, uint8_t remainder_bit_size)
{
    std::mt19937 gen(seed);
    membership::approximate::RSQF<UnderlyingType> qf(hash_bit_size, remainder_bit_size);
    std::cerr << "testing RSQF<" << toolbox::type_name<UnderlyingType>() << "> :\n";
    std::cerr << "\twith r = " << static_cast<std::size_t>(remainder_bit_size) << "\n";
    std::cerr << "\thash bit width = " << static_cast<std::size_t>(hash_bit_size) << "\n";
    std::cerr << "\tcapacity = " << qf.capacity() << "\n";
    for (std::size_t i = 1; i < 2; ++i) {//i < std::log10(10 * qf.capacity()); ++i) {
        auto nins = std::pow(10, i);
        std::cerr << "Checking filter for " << nins << " insertions\n";
        test_insertions_and_queries(qf, static_cast<std::size_t>(nins), gen);
    }
    qf.clear();
    assert(not qf.size());
    std::cerr << "DONE!\n";
    return 0;
}

template <typename T>
int test_qf_combinations(uint64_t seed) {
    uint8_t bitwidth_T = 8 * sizeof(T);
    for (uint8_t i = bitwidth_T / 2; i < bitwidth_T; ++i) {
        test_qf<T>(seed, bitwidth_T, i);
    }
    return 0;
}

int main(int argc, char* argv[])
{
    argparse::ArgumentParser parser(argv[0]);
    parser.add_argument("-i", "--insertions")
        .help("number of insertions")
        .scan<'d', uint64_t>()
        .default_value(uint64_t(0));
    parser.add_argument("-d", "--deletions")
        .help("mutation rate")
        .scan<'d', uint64_t>()
        .default_value(uint64_t(0));
    parser.add_argument("-s", "--seed")
        .help("random seed [42]")
        .scan<'d', uint64_t>()
        .default_value(uint64_t(42));

    parser.parse_args(argc, argv);
    auto insertions = parser.get<uint64_t>("--insertions");
    auto deletions = parser.get<uint64_t>("--deletions");
    auto seed = parser.get<uint64_t>("--seed");
    
    test_qf_combinations<uint32_t>(seed);    

    std::cerr << "Everything is OK\n";
    return 0;
}