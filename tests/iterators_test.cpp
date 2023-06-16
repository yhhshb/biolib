#include <vector>
#include <iostream>
#include <cassert>
#include "../include/member_iterator.hpp"

struct dummy_t {
    int a;
    unsigned int b;
};

int main() 
{
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
    std::cerr << "Everything is OK\n";
    return 0;
}