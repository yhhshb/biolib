#include <iostream>
#include <argparse/argparse.hpp>
#include "../bundled/prettyprint.hpp"
#include "../include/iterator/counting_iterator.hpp"
#include "../include/bit_vector.hpp"
#include "../include/invertible_bloom_lookup_table.hpp"

//TODO check equality for hashes for a buffered key and hashes computed from a bucket containing only that key

using citr = iterators::counting_iterator<std::size_t>;

template <typename UT, typename T>
void load_buffer(std::vector<UT>& buffer, T val)
{
    for (std::size_t i = 0; i < buffer.size(); ++i) {
        buffer[i] = val;
        val >>= bit::size<UT>();
    } 
}

template <typename UT, typename T>
void reconstruct_value(std::vector<UT> const& buffer, T& val)
{
    val = 0;
    for (auto itr = buffer.rbegin(); itr != buffer.rend(); ++itr) {
        val <<= bit::size<UT>();
        val |= *itr;
    }
}

/**
 * Check if hash computed from key is equal to hash of bucket containing that key
 */
template <typename CT, typename LT, typename UT>
std::size_t hash_check(std::size_t differences, std::size_t equalities, float epsilon, std::size_t seed)
{
    const std::size_t total_elements = equalities + differences;
    const auto max_bit_size = bit::msbll(total_elements + differences) + 1;
    bloom::invertible_lookup_table<CT, LT, UT> iblt(max_bit_size, 3, epsilon, 2*differences, seed);
    std::vector<UT> buffer = iblt.get_key_buffer();
    std::size_t hash_collisions_for_the_same_key = 0;
    {
        for (auto itr = citr(0); itr != citr(total_elements); ++itr) {
            load_buffer(buffer, *itr);
            auto bit_len = bit::msb(*itr).value_or(0) + 1;
            assert(iblt.empty());

            iblt.insert(buffer, bit_len);
            auto indexes_from_key = iblt.payload_to_bucket_indexes(buffer.data());
            std::sort(indexes_from_key.begin(), indexes_from_key.end());
            if (std::adjacent_find(indexes_from_key.cbegin(), indexes_from_key.cend()) != indexes_from_key.cend()) {
                ++hash_collisions_for_the_same_key;
            }
            // std::cerr << "key indexes: " << indexes_from_key << "\n";
            // for (auto idx : indexes_from_key) {
            //     auto indexes_from_bucket = iblt.bucket_idx_to_bucket_indexes(idx);
            //     std::sort(indexes_from_bucket.begin(), indexes_from_bucket.end());
            //     std::cerr << "\t" << indexes_from_bucket;
            //     if (indexes_from_key != indexes_from_bucket) std::cerr << " ***";
            //     std::cerr << "\n";
            // }
            iblt.remove(buffer, bit_len);
        }
    }
    return hash_collisions_for_the_same_key;
}

template <typename CT, typename LT, typename UT>
int simple_check(std::size_t differences, std::size_t equalities, std::size_t nhashes, float epsilon, std::size_t seed)
{
    const std::size_t total_elements = equalities + differences;
    const auto max_bit_size = bit::msbll(total_elements + differences) + 1;
    bloom::invertible_lookup_table<CT, LT, UT> iblt(max_bit_size, nhashes, epsilon, 2 * differences, seed);
    std::vector<UT> buffer = iblt.get_key_buffer();
    {
        for (auto itr = citr(0); itr != citr(total_elements); ++itr) {
            load_buffer(buffer, *itr);
            iblt.insert(buffer, bit::msb(*itr).value_or(0) + 1);
        }
    }

    // std::cerr << "positive:\n" << iblt << "\n";

    auto diff = iblt.generate_empty();
    {
        auto other = iblt.generate_empty();
        for (auto itr = citr(total_elements); itr != citr(total_elements + differences); ++itr) {
            load_buffer(buffer, *itr);
            other.insert(buffer, bit::msbll(*itr) + 1);
        }
        for (auto itr = citr(differences); itr != citr(total_elements); ++itr) {
            load_buffer(buffer, *itr);
            other.insert(buffer, bit::msbll(*itr) + 1);
        }
        diff = iblt - other;
        iblt -= other;
        // std::cerr << "negative:\n" << other << "\n";
    }

    auto diff_check = iblt.generate_empty();
    {
        for (auto itr = citr(0); itr != citr(differences); ++itr) {
            load_buffer(buffer, *itr);
            diff_check.insert(buffer, bit::msb(*itr).value_or(0) + 1);
        }
        for (auto itr = citr(total_elements); itr != citr(total_elements + differences); ++itr) {
            load_buffer(buffer, *itr);
            diff_check.remove(buffer, bit::msbll(*itr) + 1);
        }
    }

    assert(iblt == diff_check);
    assert(diff == diff_check);

    // std::cerr << "difference:\n" << iblt << "\n"; 

    auto [listing_inplace, status_inplace] = iblt.peel();
    // auto [listing_copy, status_copy] = diff.peel();
    // assert(listing_copy == listing_inplace);
    // assert(status_copy == status_inplace);

    auto positive_check = bit::vector<std::size_t>(differences, false);
    auto negative_check = bit::vector<std::size_t>(differences, false);

    std::size_t val;
    if (not status_inplace) { // if peeled
        std::size_t positive_size = 0;
        std::size_t negative_size = 0;
        for (auto const& p : listing_inplace) {
            reconstruct_value(p.key, val);
            // std::cerr << "val = " << (p.sign ? "+" : "-") << val << " (" << "over " << static_cast<std::size_t>(p.bit_len) << " bit) \n";
            assert(val < differences or ((total_elements <= val) and val < (total_elements + differences)));
            if (p.sign) {
                positive_check.set(val);
                ++positive_size;
            } else {
                negative_check.set(val - total_elements);
                ++negative_size;
            }
        }
        // std::cerr << "(+, -) sizes: (" << positive_size << ", " << negative_size << ")\n";
        for (auto itr = positive_check.cbegin(); itr != positive_check.cend(); ++itr) {
            if (not *itr) throw std::runtime_error("Missing positive output");
        }
        for (auto itr = negative_check.cbegin(); itr != negative_check.cend(); ++itr) {
            if (not *itr) throw std::runtime_error("Missing negaive output");
        }
    }
    return status_inplace;
}

void print_status(int status)
{
    switch(status) {
        case 0: std::cerr << "PEELED"; break;
        case 1: std::cerr << "UNPEELABLE"; break;
        case 2: std::cerr << "INFINITE"; break;
    }
}

int main(int argc, char* argv[])
{
    argparse::ArgumentParser parser(argv[0]);
    parser.add_argument("-d", "--num-diffs")
        .help("Number of differences of one sketch compared to the other (symmetric diff is 2x) [0]")
        .scan<'u', std::size_t>()
        .default_value(std::size_t(0));
    parser.add_argument("-m", "--num-matches")
        .help("Number of equal keys between two sketches [0]")
        .scan<'u', std::size_t>()
        .default_value(std::size_t(0));
    parser.add_argument("-r", "--repetitions")
        .help("Number of hash functions [3]")
        .scan<'u', std::size_t>()
        .default_value(std::size_t(3));
    parser.add_argument("-e", "--epsilon")
        .help("Over-dimensioning factor [0.1]")
        .scan<'f', float>()
        .default_value(float(0.1));
    parser.add_argument("-t", "--trials")
        .help("Number of trials [100]")
        .scan<'u', std::size_t>()
        .default_value(std::size_t(100));
    parser.parse_args(argc, argv);

    const std::size_t ndiffs = parser.get<std::size_t>("--num-diffs");
    const std::size_t nmatches = parser.get<std::size_t>("--num-matches");
    const std::size_t number_of_hashes = parser.get<std::size_t>("--repetitions");
    const float epsilon = parser.get<float>("--epsilon");
    const std::size_t trials = parser.get<std::size_t>("--trials");

    std::size_t success = 0;
    std::size_t errored = 0;
    // simple_check<int8_t, uint8_t, uint8_t>(ndiffs, nmatches, number_of_hashes, epsilon, 94);
    for (std::size_t i = 0; i < trials; ++i) {
        // std::cerr << "hash collisions: " << hash_check<int8_t, uint8_t, uint8_t>(1000, 0, epsilon, i) << " | ";
        try {
            auto res = simple_check<int8_t, uint8_t, uint8_t>(ndiffs, nmatches, number_of_hashes, epsilon, i);
            if (res == 0) ++success;
        } catch (std::exception& e) {
            std::cerr << "Caught exception: " << e.what() << "at when using seed: " << i << "\n"; 
            ++errored;
        }
        // std::cerr << "\n";
    }
    std::cerr << "success: " << success - errored << "/" << trials << "\n";
    std::cerr << "errors: " << errored << "/" << trials << "\n";
}