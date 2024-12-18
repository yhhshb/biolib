#include <numeric>
#include <vector>
#include <string>
#include <random>
#include <iostream>
#include <cassert>
#include <argparse/argparse.hpp>
#include "../bundled/prettyprint.hpp"
#include "../include/iterator/member_iterator.hpp"
#include "../include/iterator/size_iterator.hpp"
#include "../include/iterator/sorted_merge_iterator.hpp"
#include "../include/external_memory_vector.hpp"

struct dummy_t {
    int a;
    unsigned int b;
};

template<class InputIt, class OutputIt, class UnaryOp>
constexpr //< since C++20
OutputIt my_transform(InputIt first1, InputIt last1,
                   OutputIt d_first, UnaryOp unary_op)
{
    for (; first1 != last1; ++d_first, ++first1)
        *d_first = unary_op(*first1);
    return d_first;
}

int main(int argc, char* argv[]) 
{
    argparse::ArgumentParser parser(argv[0]);
    parser.add_argument("-d", "--tmp-dir")
        .help("Temporary directory where to save vector chunks")
        .required();
    parser.parse_args(argc, argv);
    std::string tmp_dir = parser.get<std::string>("--tmp-dir");

    unsigned long i;
    std::vector<dummy_t> v;

    for (i = 0; i < 10; ++i) {
        dummy_t s = {static_cast<int>(-i), static_cast<unsigned int>(i)};
        v.push_back(s);
    }

    auto access_a = [](dummy_t const& s) {return s.a;};
    i = 0;
    for (auto itr = iterators::member_iterator(v.begin(), access_a); 
              itr != iterators::member_iterator(v.end(), access_a); 
              ++itr) {
        std::cerr << "v[" << i << "].a = " << *itr << " ";
        ++i;
    }
    std::cerr << "\n";
    auto access_b = [](dummy_t const& s) {return s.b;};
    i = 0;
    for (auto itr = iterators::member_iterator(v.begin(), access_b); 
              itr != iterators::member_iterator(v.end(), access_b); 
              ++itr) {
        std::cerr << "v[" << i << "].a = " << *itr << " ";
        ++i;
    }
    std::cerr << "\n";
    assert(*(iterators::member_iterator(v.begin(), access_b) + 3) == 3);

    std::vector<std::size_t> sv(10);
    std::iota(sv.begin(), sv.end(), 0);
    for (auto itr = iterators::size_iterator(sv.begin(), 0); itr != iterators::size_iterator(sv.begin(), sv.size()); ++itr) {
        std::cerr << *itr << " ";
    }
    std::cerr << "\n";

    { // sorted vector: complex elements
        typedef std::pair<std::vector<uint32_t>, uint64_t> value_t;
        typedef emem::external_memory_vector<value_t> emem_t;
        std::vector<value_t> check;
        std::size_t size = 10;
        std::mt19937 gen(42); // Standard mersenne_twister_engine seeded with rd()
        std::vector<emem_t> vectors;
        std::uniform_int_distribution<std::size_t> lengths(0, 10);
        std::uniform_int_distribution<uint32_t> values(0, 100);
        for (std::size_t i = 0; i < 10; ++i)
        {
            vectors.emplace_back(10000, tmp_dir, std::string("kmp_test_sorted_emv") + std::to_string(i));
            for (uint64_t k = 0; k < size; ++k) {
                std::vector<uint32_t> key;
                for (std::size_t j = 0; j < lengths(gen); ++j) {
                    key.push_back(values(gen));
                }
                auto pair = std::make_pair(key, k);
                vectors.back().push_back(pair);
                check.push_back(pair);
            }
        }
        std::vector<iterators::standalone_iterator<emem_t::const_iterator>> itr_vec;
        for (auto& em : vectors) {
            auto sa_itr = iterators::standalone::const_from(em);
            itr_vec.push_back(sa_itr);
        }
        // std::transform(
        //     vectors.begin(), 
        //     vectors.end(), 
        //     itr_vec.begin(), 
        //     [](auto const& v) {return iterators::standalone::const_from(v);}
        // );
        std::vector<value_t> checked;
        iterators::sorted_merge_iterator<emem_t::const_iterator> end;
        for (auto itr = iterators::sorted_merge_iterator(itr_vec); itr != end; ++itr) {
            checked.push_back(*itr);
        }
        std::sort(check.begin(), check.end());
        assert(check.size());
        assert(check == checked);
    }
    std::cerr << "Everything is OK\n";
    return 0;
}