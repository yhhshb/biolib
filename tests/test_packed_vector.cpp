#include <random>
#include <cmath>
#include <iostream>
#include <cassert>
#include "../bundled/prettyprint.hpp"
#include "../include/packed_vector.hpp"

template <std::size_t L, typename T>
void check_packed_vector(size_t seed, size_t vector_size);

int main()
{
    using namespace std;
    const size_t seed = 42;
    const size_t vector_size = 1000;

    //
    check_packed_vector<1, uint8_t>(seed, vector_size);
    check_packed_vector<2, uint8_t>(seed, vector_size);
    check_packed_vector<3, uint8_t>(seed, vector_size);
    check_packed_vector<4, uint8_t>(seed, vector_size);
    check_packed_vector<5, uint8_t>(seed, vector_size);
    check_packed_vector<6, uint8_t>(seed, vector_size);
    check_packed_vector<7, uint8_t>(seed, vector_size);
    check_packed_vector<8, uint8_t>(seed, vector_size);

    //
    check_packed_vector<1, uint16_t>(seed, vector_size);
    check_packed_vector<2, uint16_t>(seed, vector_size);
    check_packed_vector<3, uint16_t>(seed, vector_size);
    check_packed_vector<4, uint16_t>(seed, vector_size);
    check_packed_vector<5, uint16_t>(seed, vector_size);
    check_packed_vector<6, uint16_t>(seed, vector_size);
    check_packed_vector<7, uint16_t>(seed, vector_size);
    check_packed_vector<8, uint16_t>(seed, vector_size);

    check_packed_vector<9, uint16_t>(seed, vector_size);
    check_packed_vector<10, uint16_t>(seed, vector_size);
    check_packed_vector<11, uint16_t>(seed, vector_size);
    check_packed_vector<12, uint16_t>(seed, vector_size);
    check_packed_vector<13, uint16_t>(seed, vector_size);
    check_packed_vector<14, uint16_t>(seed, vector_size);
    check_packed_vector<15, uint16_t>(seed, vector_size);
    check_packed_vector<16, uint16_t>(seed, vector_size);

    //
    check_packed_vector<1, uint32_t>(seed, vector_size);
    check_packed_vector<2, uint32_t>(seed, vector_size);
    check_packed_vector<3, uint32_t>(seed, vector_size);
    check_packed_vector<4, uint32_t>(seed, vector_size);
    check_packed_vector<5, uint32_t>(seed, vector_size);
    check_packed_vector<6, uint32_t>(seed, vector_size);
    check_packed_vector<7, uint32_t>(seed, vector_size);
    check_packed_vector<8, uint32_t>(seed, vector_size);

    check_packed_vector<9, uint32_t>(seed, vector_size);
    check_packed_vector<10, uint32_t>(seed, vector_size);
    check_packed_vector<11, uint32_t>(seed, vector_size);
    check_packed_vector<12, uint32_t>(seed, vector_size);
    check_packed_vector<13, uint32_t>(seed, vector_size);
    check_packed_vector<14, uint32_t>(seed, vector_size);
    check_packed_vector<15, uint32_t>(seed, vector_size);
    check_packed_vector<16, uint32_t>(seed, vector_size);

    check_packed_vector<17, uint32_t>(seed, vector_size);
    check_packed_vector<18, uint32_t>(seed, vector_size);
    check_packed_vector<19, uint32_t>(seed, vector_size);
    check_packed_vector<20, uint32_t>(seed, vector_size);
    check_packed_vector<21, uint32_t>(seed, vector_size);
    check_packed_vector<22, uint32_t>(seed, vector_size);
    check_packed_vector<23, uint32_t>(seed, vector_size);
    check_packed_vector<24, uint32_t>(seed, vector_size);

    check_packed_vector<25, uint32_t>(seed, vector_size);
    check_packed_vector<26, uint32_t>(seed, vector_size);
    check_packed_vector<27, uint32_t>(seed, vector_size);
    check_packed_vector<28, uint32_t>(seed, vector_size);
    check_packed_vector<29, uint32_t>(seed, vector_size);
    check_packed_vector<30, uint32_t>(seed, vector_size);
    check_packed_vector<31, uint32_t>(seed, vector_size);
    check_packed_vector<32, uint32_t>(seed, vector_size);

    //
    check_packed_vector<1, uint64_t>(seed, vector_size);
    check_packed_vector<2, uint64_t>(seed, vector_size);
    check_packed_vector<3, uint64_t>(seed, vector_size);
    check_packed_vector<4, uint64_t>(seed, vector_size);
    check_packed_vector<5, uint64_t>(seed, vector_size);
    check_packed_vector<6, uint64_t>(seed, vector_size);
    check_packed_vector<7, uint64_t>(seed, vector_size);

    check_packed_vector<8, uint64_t>(seed, vector_size);
    check_packed_vector<9, uint64_t>(seed, vector_size);
    check_packed_vector<10, uint64_t>(seed, vector_size);
    check_packed_vector<11, uint64_t>(seed, vector_size);
    check_packed_vector<12, uint64_t>(seed, vector_size);
    check_packed_vector<13, uint64_t>(seed, vector_size);
    check_packed_vector<14, uint64_t>(seed, vector_size);
    check_packed_vector<15, uint64_t>(seed, vector_size);

    check_packed_vector<16, uint64_t>(seed, vector_size);
    check_packed_vector<17, uint64_t>(seed, vector_size);
    check_packed_vector<18, uint64_t>(seed, vector_size);
    check_packed_vector<19, uint64_t>(seed, vector_size);
    check_packed_vector<20, uint64_t>(seed, vector_size);
    check_packed_vector<21, uint64_t>(seed, vector_size);
    check_packed_vector<22, uint64_t>(seed, vector_size);
    check_packed_vector<23, uint64_t>(seed, vector_size);

    check_packed_vector<24, uint64_t>(seed, vector_size);
    check_packed_vector<25, uint64_t>(seed, vector_size);
    check_packed_vector<26, uint64_t>(seed, vector_size);
    check_packed_vector<27, uint64_t>(seed, vector_size);
    check_packed_vector<28, uint64_t>(seed, vector_size);
    check_packed_vector<29, uint64_t>(seed, vector_size);
    check_packed_vector<30, uint64_t>(seed, vector_size);
    check_packed_vector<31, uint64_t>(seed, vector_size);

    check_packed_vector<32, uint64_t>(seed, vector_size);
    check_packed_vector<33, uint64_t>(seed, vector_size);
    check_packed_vector<34, uint64_t>(seed, vector_size);
    check_packed_vector<35, uint64_t>(seed, vector_size);
    check_packed_vector<36, uint64_t>(seed, vector_size);
    check_packed_vector<37, uint64_t>(seed, vector_size);
    check_packed_vector<38, uint64_t>(seed, vector_size);
    check_packed_vector<39, uint64_t>(seed, vector_size);

    check_packed_vector<40, uint64_t>(seed, vector_size);
    check_packed_vector<41, uint64_t>(seed, vector_size);
    check_packed_vector<42, uint64_t>(seed, vector_size);
    check_packed_vector<43, uint64_t>(seed, vector_size);
    check_packed_vector<44, uint64_t>(seed, vector_size);
    check_packed_vector<45, uint64_t>(seed, vector_size);
    check_packed_vector<46, uint64_t>(seed, vector_size);
    check_packed_vector<47, uint64_t>(seed, vector_size);

    check_packed_vector<48, uint64_t>(seed, vector_size);
    check_packed_vector<49, uint64_t>(seed, vector_size);
    check_packed_vector<50, uint64_t>(seed, vector_size);
    check_packed_vector<51, uint64_t>(seed, vector_size);
    check_packed_vector<52, uint64_t>(seed, vector_size);
    check_packed_vector<53, uint64_t>(seed, vector_size);
    check_packed_vector<54, uint64_t>(seed, vector_size);
    check_packed_vector<55, uint64_t>(seed, vector_size);

    check_packed_vector<56, uint64_t>(seed, vector_size);
    check_packed_vector<57, uint64_t>(seed, vector_size);
    check_packed_vector<58, uint64_t>(seed, vector_size);
    check_packed_vector<59, uint64_t>(seed, vector_size);
    check_packed_vector<60, uint64_t>(seed, vector_size);
    check_packed_vector<61, uint64_t>(seed, vector_size);
    check_packed_vector<62, uint64_t>(seed, vector_size);
    check_packed_vector<63, uint64_t>(seed, vector_size);

    check_packed_vector<64, uint64_t>(seed, vector_size);

    cerr << "Everything is OK\n";
    return 0;
}

template <std::size_t L, typename T>
void check_packed_vector(size_t seed, size_t vector_size)
{
    static_assert(L <= (8 * sizeof(T)));
    std::mt19937 gen(seed); // Standard mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<std::size_t> distrib(0, (1ULL << (L == 8*sizeof(T) ? 0 : L))- 1);//std::pow(static_cast<std::size_t>(2), L) - 1);
    bit::packed::vector<T> pv(L);
    std::vector<std::size_t> check;
    for (std::size_t i = 0; i < vector_size; ++i) {
        auto val = distrib(gen);
        pv.push_back(val);
        check.push_back(val);
    }
    // std::cerr << check << "\n";
    // for (auto itr = pv.vector_data().cbegin(); itr != pv.vector_data().cend(); ++itr) std::cerr << uint16_t(*itr) << " ";
    // std::cerr << "\n";
    // std::cerr << "size = " << pv.size() << std::endl;
    for (std::size_t i = 0; i < vector_size; ++i) {
        // std::cerr << pv.template at<std::size_t>(i) << ", ";
        // std::cerr << check.at(i) << "\n";
        assert(pv.template at<T>(i) == check.at(i));
    }
    std::cerr << "bit width " << L << " over " << sizeof(T) * 8 << ", done\n";
}