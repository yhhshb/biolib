#include "../include/invertible_bloom_lookup_table.hpp"
#include <cmath>

namespace bloom {

const std::array<float, 8> invertible_lookup_table::ck_table = {0, 0, 0, 1.222, 1.295, 1.425, 1.570, 1.721};

invertible_lookup_table::invertible_lookup_table(
    uint8_t counter_bit_size, 
    uint8_t payload_bit_size, 
    uint8_t repetitions, 
    float epsilon, 
    std::size_t expected_number_of_differences,
    uint64_t seed
) : cnt_size(counter_bit_size), pld_size(payload_bit_size), mseed(seed), nreps(repetitions), eps(epsilon)
{
    if (cnt_size > 8 * sizeof(std::size_t)) throw std::invalid_argument("[IBLT] counter bit size can't be > than maximum word size");
    const std::size_t underlying_bit_size = 8 * sizeof(decltype(buckets)::value_type);
    uint16_t total_bit_size = (static_cast<uint16_t>(cnt_size) + pld_size);
    bucket_byte_size = total_bit_size / underlying_bit_size + ((total_bit_size % underlying_bit_size) == 0 ? 0 : 1);
    chunk_size = static_cast<std::size_t>(std::ceil((static_cast<double>(eps) + ck_table[nreps])) * expected_number_of_differences / nreps + 1);
    buckets.resize(bucket_byte_size * chunk_size * nreps);
}

} // namespace bloom