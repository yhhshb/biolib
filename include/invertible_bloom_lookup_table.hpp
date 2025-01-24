#ifndef INVERTIBLE_BLOOM_LOOKUP_TABLE_HPP
#define INVERTIBLE_BLOOM_LOOKUP_TABLE_HPP

#include <cstddef>
#include <cstdint>
#include <optional>
#include <set>
#include "bit_operations.hpp"
#include "hash.hpp"

namespace bloom {

const std::array<float, 8> ck_table = {0, 0, 0, 1.222, 1.295, 1.425, 1.570, 1.721};

#define CLASS_HEADER template <typename CounterType, typename UnderlyingType>
#define METHOD_HEADER invertible_lookup_table<CounterType, UnderlyingType>

template <typename CounterType, typename UnderlyingType = bit::max_width_native_type>
class invertible_lookup_table
{
    public:
        typedef uint16_t key_bit_len_t;
        struct record_t {
            record_t (std::vector<UnderlyingType> keyval, bl) : key(keyval), bit_len(bl);
            std::vector<UnderlyingType> key;
            key_bit_len_t bit_len;
        };
        invertible_lookup_table(
            key_bit_len_t payload_bit_size, // 256 bits
            uint8_t repetitions, 
            float epsilon, 
            std::size_t expected_number_of_differences,
            uint64_t seed
        );
        void insert(UnderlyingType const * const ptr, std::size_t len) {modify(ptr, len, true);}
        void remove(UnderlyingType const * const ptr, std::size_t len) {modify(ptr, len, false);}
        void clear();
        std::vector<record_t> peel();
        std::size_t size() const noexcept {return nreps * chunk_size;}
        invertible_lookup_table& operator-=(invertible_lookup_table const& other);

        template <class Visitor>
        void visit(Visitor& visitor) const {
            visitor.visit(pld_bit_size);
            visitor.visit(nreps);
            visitor.visit(eps);
            visitor.visit(mseed);
            visitor.visit(chunk_size);
            visitor.visit(bucket_byte_size);
            visitor.visit(counters);
            visitor.visit(lengths);
            visitor.visit(buckets);
        }

        template <class Visitor>
        invertible_lookup_table load(Visitor& visitor) {
            invertible_lookup_table table;
            table.visit(visitor);
            return table;
        }

    private:
        friend invertible_lookup_table operator-(invertible_lookup_table const& first, invertible_lookup_table const& second);
        invertible_lookup_table() : pld_bit_size(0), mseed(0), nreps(0), eps(0) {};
        void modify(UnderlyingType const * const ptr, key_bit_len_t len, bool addition);
        void aligned_xor(std::size_t bucket_idx, UnderlyingType const * const ptr, std::size_t bit_len, key_bit_len_t ptr_ut_len) const;
        void get_at(std::size_t bucket_idx); // not const because it modifies buffer
        std::pair<std::size_t, std::size_t> bucket_idx_to_ut_idx(std::size_t bucket_idx) const noexcept;
        // std::optional<std::size_t> find_peelable_bucket(std::size_t bucket_idx) const noexcept;
        template <class Visitor>
        void visit(Visitor& visitor) 
        {
            visitor.visit(pld_bit_size);
            visitor.visit(nreps);
            visitor.visit(eps);
            visitor.visit(mseed);
            visitor.visit(chunk_size);
            visitor.visit(bucket_byte_size);
            visitor.visit(counters);
            visitor.visit(lengths);
            visitor.visit(buckets);
        }

        static const std::array<float, 8> ck_table;
        key_bit_len_t pld_bit_size;
        uint8_t nreps;
        float eps;
        std::size_t mseed;
        std::size_t chunk_size;
        std::size_t bucket_byte_size;
        std::vector<CounterType> counters;
        std::vector<decltype(pld_bit_size)> lengths;
        std::vector<UnderlyingType> buckets;
        std::vector<UnderlyingType> buffer; // buffer for xor operations
};

CLASS_HEADER
METHOD_HEADER::invertible_lookup_table(
    key_bit_len_t payload_bit_size, 
    uint8_t repetitions, 
    float epsilon, 
    std::size_t expected_number_of_differences, 
    uint64_t seed
) : pld_bit_size(payload_bit_size), mseed(seed), nreps(repetitions), eps(epsilon)
{
    chunk_size = static_cast<std::size_t>(std::ceil((static_cast<double>(eps) + ck_table[nreps])) * expected_number_of_differences / nreps + 1);
    auto total_payload_bit_size = pld_bit_size * size();
    buckets.resize(bit::round_up2(total_payload_bit_size, bit::size<UnderlyingType>::value);
    counters.resize(size());
    lengths.resize(size());
    buckets.shrink_to_fit();
    counters.shrink_to_fit();
    lengths.shrink_to_fit();
    clear();
}

CLASS_HEADER
void 
METHOD_HEADER::clear()
{
    auto s = size();
    for (std::size_t i = 0; i < s; ++i) counters[i] = 0;
    for (std::size_t i = 0; i < s; ++i) lengths[i] = 0;
    for (std::size_t i = 0; i < buckets.size(); ++i) buckets[i] = 0;
}

CLASS_HEADER
std::vector<typename METHOD_HEADER::record_t> 
METHOD_HEADER::peel()
{
    std::array<std::size_t, 1 << bit::size<decltype(nreps)>::value> npi; // new peelable indexes
    std::size_t npi_size = 0;
    std::set<std::size_t> peelable_indexes;
    std::set<std::size_t> done;
    for (std::size_t i = 0; i < size(); ++i) if (counters[i] == 1 or counters[i] == -1) peelable_indexes.insert(i);
    std::vector<record_t> results;
    while (peelable_indexes.size() > 0) { // no max passes threshold, we want to do better than 2 years ago
        std::size_t bucket_idx;
        {
            auto itr = peelable_indexes.cbegin();
            bucket_idx = *itr;
            peelable_indexes.erase(itr);
        }
        get_at(bucket_idx); // now buffer contains the contents of the bucket
        bool ok = false;
        {
            auto master_hash = hash::hash64::hash(reinterpret_cast<uint8_t const*>(buffer.data()), bit::roundup2(pld_bit_size, bit::size<UnderlyingType>::value), seed);
            for (std::size_t i = 0; i < nreps; ++i) {
                auto check = hash::hash64::hash(master_hash, i) % chunk_size + i * chunk_size;
                if (bucket_idx == check) ok = true;
                npi[npi_size++] = check;
            }
        }
        if (ok) {
            done.insert(bucket_idx);
            results.emplace_back(std::move(buffer), lengths.at(bucket_idx));
            for (std::size_t i = 0; i < npi_size; ++i) {
                aligned_xor(npi[i], original_key.data(), original_key.size(), bit_len);
                if (done.find(npi[i]) == done.end() and (counters[npi[i]] == 1 or counters[npi[i]] == -1)) {
                    peelable_indexes.insert(npi[i]);
                }
            }
        }
    }
    return results;
}

CLASS_HEADER
void 
METHOD_HEADER::modify(UnderlyingType const * const ptr, key_bit_len_t bit_len, bool addition)
{
    buffer.reserve(pld_bit_size / bit::size<UnderlyingType>::value + 1);
    const auto ptr_ut_len = bit::round_up2(bit_len, bit::size<UnderlyingType>::value) / sizeof(UnderlyingType);
    auto master_hash = hash::hash64::hash(reinterpret_cast<uint8_t const*>(ptr), ptr_ut_len, seed);
    for (std::size_t i = 0; i < nreps; ++i) {
        auto bucket_idx = hash::hash64::hash(master_hash, i) % chunk_size + i * chunk_size;
        auto ut_idx = aligned_xor(bucket_idx, ptr, ptr_ut_len, bit_len);
        if (addition) ++counters[bucket_idx];
        else --counters[bucket_idx];
    }
}

/**
 * ptr is a pointer to a sequence of Underlying type right-aligned:aligned
 * example with UnderlyingType = uint8_t and a value of 11 bits, numbers are bit indexes
 * [7 6 5 4 3 2 1 0], [* * * * * 10 9 8]
 */
CLASS_HEADER
void 
METHOD_HEADER::aligned_xor(std::size_t bucket_idx, UnderlyingType const * const ptr, std::size_t ptr_ut_len, key_bit_len_t bit_len) const
{
    assert(bit_len <= pld_bit_size);
    auto [ut_idx, carry_length] = bucket_idx_to_ut_idx(bucket_idx);
    const auto fitting_length = (bit::size<UnderlyingType>::value - carry_length);
    const auto mask = (UnderlyingType(1) << fitting_length) - 1;
    buffer.clear();

    UnderlyingType carry = 0;
    for (std::size_t i = 0; i < ptr_ut_len; ++i) { 
        buffer.push_back(carry | (ptr[i] & mask)); // this actually breaks input bit-order but it's faster to compute
        carry = ptr[i] & ~mask;
        bit_len -= fitting_length;
        if (i != 0) bit_len -= carry_length;
    }
    // the last carry needs special handling since its length is the number of remaining bits to be packed
    buffer.push_back(carry << (bit::size<UnderlyingType>::value - bit_len));

    for (std::size_t i = 0; i < buffer.size(); ++i) { // TODO modify buckets in-place after debug (remove buffer usage)
        buckets[ut_idx] ^= buffer[i];
        ++ut_idx;
    }
}

/**
 * Retrieve a key from a bucket.
 * The resulting key is stored into buffer with its original alignment (right) preserved.  
 */
CLASS_HEADER
void 
METHOD_HEADER::get_at(std::size_t bucket_idx) 
{
    // copy contents of bucket into buffer by reconstructing its original alignment
    assert(counters.at(bucket_idx) == 1 or counters.at(bucket_idx) == -1);
    auto [ut_idx, carry_length] = bucket_idx_to_ut_idx(bucket_idx);
    const auto fitting_length = (bit::size<UnderlyingType>::value - carry_length);
    const auto mask = (UnderlyingType(1) << fitting_length) - 1;
    buffer.clear();

    auto ut_end_idx = ut_idx + (pld_bit_size + carry_length) / bit::size<UnderlyingType>::value;
    for (std::size_t i = ut_idx; i < ut_end_idx; ++i) {
        buffer.push_back(buckets.at(i) & mask) | (buckets.at(i + 1) & ~mask);
    }
    auto back_shift = bit::round_up2<std::size_t>(pld_bit_size + carry_length, bit::size<UnderlyingType>::value) - (pld_bit_size + carry_length);
    buffer.push_back(buckets >> (bit::size<UnderlyingType>::value - back_shift));
}

CLASS_HEADER
std::pair<std::size_t, std::size_t> // index, shift 
METHOD_HEADER::bucket_idx_to_ut_idx(std::size_t bucket_idx) const noexcept
{
    auto bit_idx = bucket_idx * pld_bit_size;
    std::make_pair(bit_idx / bit::size<UnderlyingType>::value, bit_idx % bit::size<UnderlyingType>::value);
}

// CLASS_HEADER
// std::optional<std::size_t> 
// METHOD_HEADER::find_peelable_bucket(std::size_t bucket_idx) const noexcept // bucket_t const * const buckets, uint64_t blen, uint64_t * const last
// {
// 	auto start = bucket_idx;
// 	bool empty = true;
// 	for(;bucket_idx < size(); ++bucket_idx) {
// 		if (counters[bucket_idx] == 1 or counters[bucket_idx] == -1) return bucket_idx;
// 		else if (counters[bucket_idx] != 0) empty = false;
// 	}
// 	for(bucket_idx = 0; bucket_idx < start; ++bucket_idx) {
// 		if (buckets[*last].counter == 1 || buckets[*last].counter == -1) return bucket_idx;
// 		else if (counters[bucket_idx] != 0) empty = false;
// 	}
// 	if (empty) = return size();
// 	return std::null_opt;
// }

#undef METHOD_HEADER
#undef CLASS_HEADER

} // namespace bloom

#endif // INVERTIBLE_BLOOM_LOOKUP_TABLE_HPP