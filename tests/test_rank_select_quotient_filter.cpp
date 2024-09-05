#include <random>
#include <iostream>
#include <cassert>
#include <vector>
#include <argparse/argparse.hpp>
#include "../include/toolbox.hpp"
#include "../include/rank_select_quotient_filter.hpp"

void print_qr(std::size_t x, std::size_t remainder_bitwidth)
{
    std::cerr << (x >> remainder_bitwidth) << " | " << (x & ((1ULL << remainder_bitwidth) - 1)) << "\n";
}

template <class QF>
int test_insertions_and_queries(QF& qf, std::size_t insertions, std::mt19937& generator, bool debug)
{
    std::uniform_int_distribution<std::size_t> distribution(0, (1ULL << qf.hash_bitwidth()) - 1);
    std::vector<uint64_t> check;
    if (debug) std::cerr << ">>> Insertion\n";
    for (std::size_t i = 0; i < insertions; ++i) {
        auto x = distribution(generator);
        check.push_back(x);
        qf.insert(x);
        if (debug) {
            qf.print_metadata();
            qf.print_remainders();
        }
    }
    if (debug) {
        std::cerr << ">>> Query\n";
        // for (auto e : check) print_qr(e, qf.remainder_bitwidth());
    }
    for (auto x : check) {
        assert(qf.contains(x));
    }
    if (debug) std::cerr << ">>> Iterator\n";
    std::multiset itr_checker(check.begin(), check.end());
    std::size_t c = 0;
    for (auto itr = qf.cbegin(); itr != qf.cend(); ++itr) {
        auto val = *itr;
        assert(qf.contains(val));
        assert(itr_checker.count(val) > 0);
        itr_checker.erase(val);
        ++c;
    }
    if (itr_checker.size()) {
        std::cerr << "qf.size() = " << qf.size() << ", seen " << c << " elements, remaining " << itr_checker.size() << ":\n";
        for (auto e : itr_checker) print_qr(e, qf.remainder_bitwidth());
    }
    assert(itr_checker.empty());
    if (debug) std::cerr << ">>> Deletion\n";
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
    test_insertions_and_queries(qf, static_cast<std::size_t>(qf.capacity() / 2), gen, false);
    qf.clear();
    test_insertions_and_queries(qf, static_cast<std::size_t>(qf.max_load_factor() * qf.capacity() * 0.9), gen, false);
    qf.clear();
    test_insertions_and_queries(qf, static_cast<std::size_t>(qf.capacity() * qf.max_load_factor() * 2), gen, false);
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
    
    // test_qf_combinations<uint32_t>(seed);
    // test_qf<uint32_t>(seed, 32, 32);
    // test_qf<uint32_t>(seed, 32, 31);
    test_qf<uint32_t>(seed, 32, 27);
    // test_qf<uint32_t>(seed, 32, 25);

    std::cerr << "Everything is OK\n";
    return 0;
}