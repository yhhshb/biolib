#include <random>
#include <iostream>
#include <cassert>
#include <unordered_set>
#include <argparse/argparse.hpp>
#include "../include/rank_select_quotient_filter.hpp"

template <class QF>
int test_insertions_and_queries(QF& qf, std::size_t insertions, std::mt19937& generator)
{
    std::uniform_int_distribution<std::size_t> distribution(0, (1ULL << qf.hash_bitwidth()) - 1);
    std::unordered_set<uint64_t> check;
    fprintf(stderr, ">>> Insertion\n");
    for (std::size_t i = 0; i < insertions; ++i) {
        auto x = distribution(generator);
        check.insert(x);
        qf.insert(x);
        // assert(qf.contains(x));
    }
    fprintf(stderr, ">>> Query\n");
    for (auto x : check) {
        assert(qf.contains(x));
    }
    fprintf(stderr, ">>> Deletion\n");
    for (auto itr = qf.cbegin(); itr != qf.cend(); ++itr) {
        check.erase(*itr);
    }
    assert(check.size() == 0);
    return 0;
}

template <class QF>
int test_user_parameters(QF& qf, std::size_t insertions, std::size_t deletions)
{
    // TODO
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
    std::mt19937 gen(seed);

    membership::approximate::RSQF<uint64_t> qf(32, 20);
    std::cerr << "QF capacity = " << qf.capacity() << ", size = " << qf.size() << "\n";
    for (std::size_t i = 1; i < 2; ++i) {
        auto nins = std::pow(10, i);
        std::cerr << "Checking filter for " << nins << " insertions\n";
        test_insertions_and_queries(qf, static_cast<std::size_t>(nins), gen);
        qf.clear();
        assert(not qf.size());
    }

    std::cerr << "Everuthing OK\n";
    return 0;
}