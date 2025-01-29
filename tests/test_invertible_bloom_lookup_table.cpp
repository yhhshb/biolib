#include <iostream>
#include "../include/iterator/counting_iterator.hpp"
#include "../include/bit_vector.hpp"
#include "../bundled/prettyprint.hpp"
#include "../include/invertible_bloom_lookup_table.hpp"

template <typename CT, typename LT, typename UT>
int simple_check(std::size_t differences, std::size_t equalities, float epsilon)
{
    using citr = iterators::counting_iterator<std::size_t>;
    const std::size_t total_elements = equalities + differences;
    const auto max_bit_size = bit::msbll(total_elements + differences) + 1;
    std::vector<UT> buffer = {0};
    bloom::invertible_lookup_table<CT, LT, UT> iblt(max_bit_size, 3, epsilon, 2*differences, 42);
    {
        // std::vector<UT> original = {0,1,2,3,4,5,6,7,8,9,10};
        for (auto itr = citr(0); itr != citr(total_elements); ++itr) {
            buffer[0] = *itr;
            iblt.insert(buffer, bit::msb(*itr).value_or(0) + 1);
        }
    }

    auto diff = iblt.generate_empty();
    {
        auto other = iblt.generate_empty();
        // std::vector<UT> mutated = {0,1,2,11,4,5,12,7,8,9,13};
        for (auto itr = citr(total_elements); itr != citr(total_elements + differences); ++itr) {
            buffer[0] = *itr;
            other.insert(buffer, bit::msbll(*itr) + 1);
        }
        for (auto itr = citr(differences); itr != citr(total_elements); ++itr) {
            buffer[0] = *itr;
            other.insert(buffer, bit::msbll(*itr) + 1);
        }
        diff = iblt - other;
        iblt -= other;
    }
    auto [listing_inplace, status_inplace] = iblt.peel();
    auto [listing_copy, status_copy] = diff.peel();
    assert(listing_copy == listing_inplace);
    assert(status_copy == status_inplace);

    auto positive_check = bit::vector<std::size_t>(differences, false);
    auto negative_check = bit::vector<std::size_t>(differences, false);
    for (auto const& p : listing_inplace) {
        std::size_t val = *reinterpret_cast<std::size_t const*>(p.key.data());
        std::cerr << "val = " << (p.sign ? "+" : "-") << val << "\n";
        if (p.sign) {
            positive_check.set(val);
        } else {
            negative_check.set(val - total_elements);
        }
    }
    for (auto itr = positive_check.cbegin(); itr != positive_check.cend(); ++itr) {
        assert(status_inplace or *itr);
    }
    for (auto itr = negative_check.cbegin(); itr != negative_check.cend(); ++itr) {
        assert(status_inplace or *itr);
    }
    return status_inplace;
}

void print_status(int status)
{
    std::cerr << "sketch status: ";
    switch(status) {
        case 0: std::cerr << "PEELED"; break;
        case 1: std::cerr << "UNPEELABLE"; break;
        case 2: std::cerr << "INFINITE"; break;
    }
    std::cerr << "\n";
}

int main()
{
    const float epsilon = 1;
    // print_status(simple_check<int8_t, uint8_t, uint8_t>(3, 7, epsilon));
    print_status(simple_check<int8_t, uint8_t, uint16_t>(3, 7, epsilon));
    print_status(simple_check<int8_t, uint8_t, uint32_t>(3, 7, epsilon));
    print_status(simple_check<int8_t, uint8_t, uint64_t>(3, 7, epsilon));
}