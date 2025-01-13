#include <numeric>
#include <vector>
#include <string>
#include <random>
#include <iostream>
#include <cassert>
#include <argparse/argparse.hpp>
#include "../bundled/prettyprint.hpp"

#include "../include/external_memory_vector.hpp"
#include "../include/iterator/member_iterator.hpp"
#include "../include/iterator/size_iterator.hpp"
#include "../include/iterator/counting_iterator.hpp"
#include "../include/iterator/sorted_merge_iterator.hpp"

void test_member_iterator();
void test_size_iterator();
void test_sorted_merge_iterator(std::string const& tmp_dir);
void test_sorted_merge_iterator_space();

int main(int argc, char* argv[]) 
{
    argparse::ArgumentParser parser(argv[0]);
    parser.add_argument("-d", "--tmp-dir")
        .help("Temporary directory where to save vector chunks")
        .required();
    parser.parse_args(argc, argv);
    std::string tmp_dir = parser.get<std::string>("--tmp-dir");

    test_member_iterator();
    test_size_iterator();
    test_sorted_merge_iterator(tmp_dir);
    test_sorted_merge_iterator_space();
    std::cerr << "Everything is OK\n";
    return 0;
}

void test_member_iterator()
{
    struct dummy_t {
        int a;
        unsigned int b;
    };
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
              ++itr
    ) {
        assert(i == -(*itr));
        ++i;
    }

    auto access_b = [](dummy_t const& s) {return s.b;};
    i = 0;
    for (auto itr = iterators::member_iterator(v.begin(), access_b); 
              itr != iterators::member_iterator(v.end(), access_b); 
              ++itr
    ) {
        assert(i == *itr);
        ++i;
    }
    assert(*(iterators::member_iterator(v.begin(), access_b) + 3) == 3);
}

void test_size_iterator()
{
    std::vector<std::size_t> sv(10);
    std::iota(sv.begin(), sv.end(), 0);
    std::size_t i = 0;
    for (auto itr = iterators::size_iterator(sv.begin(), 0); itr != iterators::size_iterator(sv.begin(), sv.size()); ++itr) {
        assert(i == itr.get_idx());
        ++i;
    }
}

void test_sorted_merge_iterator(std::string const& tmp_dir)
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
    std::vector<value_t> tobe_checked;
    iterators::sorted_merge_iterator<emem_t::const_iterator> end;
    for (auto itr = iterators::sorted_merge_iterator(itr_vec); itr != end; ++itr) {
        tobe_checked.push_back(*itr);
    }
    std::sort(check.begin(), check.end());
    assert(check.size());
    assert(check == tobe_checked);
}

template <typename T>
iterators::standalone_iterator<iterators::counting_iterator<T>>
from_range(T start, T stop)
{
    using namespace iterators;
    return standalone_iterator(counting_iterator(start), counting_iterator(stop));
}

void test_sorted_merge_iterator_space()
{
    const std::size_t range = 1000;
    typedef iterators::standalone_iterator<iterators::counting_iterator<std::size_t>> itr_t;
    std::vector<itr_t> iterator_list;
    for (std::size_t i = 0; i < 10; ++i) { // Memory usage depends on the number of iterators we are merging
        iterator_list.push_back(from_range(i * range, (i+1) * range));
    }
    iterators::sorted_merge_iterator<iterators::counting_iterator<std::size_t>> end;
    std::size_t i = 0;
    for (auto itr = iterators::sorted_merge_iterator(iterator_list); itr != end; ++itr) {
        assert(i == *itr);
        ++i;
    }
}