#ifndef INVERTIBLE_BLOOM_LOOKUP_TABLE_HPP
#define INVERTIBLE_BLOOM_LOOKUP_TABLE_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_set>
#include <ostream>
#include "bit_operations.hpp"
#include "hash.hpp"

namespace bloom {

namespace constants {
    const std::array<float, 8> ilt_scaling_table = {0, 0, 0, 1.222, 1.295, 1.425, 1.570, 1.721};
    enum peel_status_t {PEELED, UNPEELABLE, INFINITE};
}

#define CLASS_HEADER template <typename CounterType, typename LengthType, typename UnderlyingType>
#define METHOD_HEADER invertible_lookup_table<CounterType, LengthType, UnderlyingType>

template <typename CounterType, typename LengthType, typename UnderlyingType = bit::max_width_native_type>
class invertible_lookup_table
{
    public:
        struct record_t {
            record_t () : bit_len(0), sign(true) {}
            record_t (std::vector<UnderlyingType> keyval, LengthType bl, bool source) : key(keyval), bit_len(bl), sign(source) {}
            std::vector<UnderlyingType> key;
            LengthType bit_len;
            bool sign;
            friend bool operator==(record_t const& a, record_t const& b) {return a.key == b.key and a.bit_len == b.bit_len and a.sign == b.sign;}
            friend bool operator!=(record_t const& a, record_t const& b) {return not (a == b);}
        };

        invertible_lookup_table(
            LengthType maximum_payload_bit_size, 
            uint8_t number_of_hash_functions, 
            float epsilon, 
            std::size_t expected_number_of_differences, 
            uint64_t seed
        );
        invertible_lookup_table (invertible_lookup_table const&) = default;
        void insert(std::vector<UnderlyingType> const& key, std::size_t bit_len) {modify(key, bit_len, true);}
        void remove(std::vector<UnderlyingType> const& key, std::size_t bit_len) {modify(key, bit_len, false);}
        void clear() {for (std::size_t i = 0; i < buckets.size(); ++i) buckets[i] = 0;}
        std::pair<std::vector<record_t>, constants::peel_status_t> peel();
        std::size_t size() const noexcept {return nbuckets;}
        invertible_lookup_table& operator-=(invertible_lookup_table const& other);
        invertible_lookup_table generate_empty() const;

        template <class Visitor> void visit(Visitor& visitor) const;
        template <class Visitor> invertible_lookup_table load(Visitor& visitor);

    private:
        struct record_hash_t { // hash function for record_t which only depends on "key" fields
            std::size_t operator()(const record_t& record) const {
                return hash::hash64::hash(
                    reinterpret_cast<uint8_t const*>(record.key.data()), 
                    sizeof(typename decltype(record.key)::value_type) * record.key.size(), 
                    0 // seed
                );
            }
        };

        struct bucket_view_t {
            CounterType* counter;
            LengthType* length_sum;
            UnderlyingType* payload_start;
        };

        invertible_lookup_table() : pld_bit_size(0), mseed(0), nreps(0), nbuckets(0), bucket_byte_size(0) {};
        void init();
        void modify(std::vector<UnderlyingType> const& key, LengthType len, bool addition);
        void aligned_xor(UnderlyingType* const payload_start, UnderlyingType const * const ptr, std::size_t ut_len);
        bucket_view_t bucket_idx_to_bucket_view(std::size_t bucket_idx) const;
        bool looks_pure(std::size_t bucket_idx, std::vector<std::size_t>& other_idxs) const;
        bool is_compatible(invertible_lookup_table const& other) const noexcept;
        std::size_t payload_len_as_number_of_underlying_type_integers() const noexcept {return bit::round_up2(static_cast<std::size_t>(pld_bit_size), bit::size<UnderlyingType>()) / bit::size<UnderlyingType>();}
        template <class Visitor> void visit(Visitor& visitor);

        friend std::ostream& operator<<(std::ostream& ostrm, invertible_lookup_table const& sketch)
        {
            const auto payload_ut_len = sketch.payload_len_as_number_of_underlying_type_integers();
            ostrm << "[";
            for (std::size_t i = 0; i < sketch.nbuckets; ++i) {
                auto bucket = sketch.bucket_idx_to_bucket_view(i);
                ostrm << "(";
                ostrm << static_cast<std::ptrdiff_t>(*bucket.counter) << ", ";
                ostrm << static_cast<std::size_t>(*bucket.length_sum) << ", [";
                for (std::size_t j = 0; j < payload_ut_len; ++j) {
                    ostrm << static_cast<std::size_t>(bucket.payload_start[j]);
                    if (j != payload_ut_len - 1) ostrm << ", ";
                }
                ostrm << "])";
                if (i != sketch.nbuckets - 1) ostrm << ", ";
            }
            ostrm << "]";
            return ostrm;
        }

        friend invertible_lookup_table operator-(invertible_lookup_table const& self, invertible_lookup_table const& other)
        {
            if (not self.is_compatible(other)) throw std::invalid_argument("[operator-=] subtrahend is incompatible");
            invertible_lookup_table toret = self;
            toret -= other;
            return toret;
        }

        LengthType pld_bit_size;
        uint8_t nreps;
        std::size_t mseed;
        std::size_t nbuckets;
        std::size_t bucket_byte_size;
        std::vector<uint8_t> buckets;
};

CLASS_HEADER
METHOD_HEADER::invertible_lookup_table(
    LengthType maximum_payload_bit_size, 
    uint8_t number_of_hash_functions, 
    float epsilon, 
    std::size_t expected_number_of_differences, 
    uint64_t seed
) : pld_bit_size(maximum_payload_bit_size), nreps(number_of_hash_functions), mseed(seed)
{
    static_assert(std::is_signed<CounterType>::value, "Counter type must be a signed integer type");
    static_assert(std::is_unsigned<LengthType>::value, "Length type must be an unsigned integer type");
    if (nreps < 3 or nreps >= constants::ilt_scaling_table.size()) throw std::invalid_argument("number of hash functions must be in [3, 7]");
    nbuckets = static_cast<std::size_t>(expected_number_of_differences * (constants::ilt_scaling_table[nreps] + static_cast<double>(epsilon)));
    std::size_t counter_byte_size = sizeof(CounterType);
    std::size_t length_byte_size = sizeof(LengthType);
    std::size_t payload_byte_size = bit::round_up2(static_cast<std::size_t>(pld_bit_size), bit::size<UnderlyingType>()) / bit::size<uint8_t>();
    bucket_byte_size = counter_byte_size + length_byte_size + payload_byte_size;
    init();
}

CLASS_HEADER
std::pair<std::vector<typename METHOD_HEADER::record_t>, constants::peel_status_t>
METHOD_HEADER::peel()
{
    const std::size_t MAX_CYCLE_COUNT = 3;
    std::vector<std::size_t> other_idxs;
    std::vector<std::size_t> peelable_indexes;
    for (std::size_t i = 0; i < nbuckets; ++i) {
        if (looks_pure(i, other_idxs)) peelable_indexes.push_back(i);
    }
    
    std::unordered_set<record_t, record_hash_t> results;
    std::vector<std::size_t> next_peelable_indexes;
    record_t record;
    const std::size_t payload_ut_len = payload_len_as_number_of_underlying_type_integers();
    record.key.resize(payload_ut_len);
    std::size_t max_output_size = 1;
    std::size_t nmax_count = 0;
    while (not peelable_indexes.empty() and nmax_count != MAX_CYCLE_COUNT) {
        next_peelable_indexes.clear();
        for (auto bucket_idx : peelable_indexes) {
            if (looks_pure(bucket_idx, other_idxs)) {
                auto bucket = bucket_idx_to_bucket_view(bucket_idx);
                record.bit_len = *bucket.length_sum;
                for (std::size_t i = 0; i < payload_ut_len; ++i) {
                    record.key[i] = bucket.payload_start[i];
                }
                if (*bucket.counter == 1) {
                    remove(record.key, record.bit_len);
                    record.sign = true;
                } else if (*bucket.counter == -1) {
                    insert(record.key, record.bit_len);
                    record.sign = false;
                } else {
                    assert(false);
                }
                { // add or remove from output
                    auto itr = results.find(record);
                    if (itr != results.end()) results.erase(itr);
                    else results.insert(record);
                    if (results.size() > max_output_size) {
                        max_output_size = results.size();
                        nmax_count = 0;
                    } else if (results.size() == max_output_size) {
                        ++nmax_count;
                    }
                }
                for (auto idx : other_idxs) {
                    std::vector<std::size_t> dummy;
                    if (looks_pure(idx, dummy)) {
                        next_peelable_indexes.push_back(idx);
                    }
                }
            }
        }
        peelable_indexes = next_peelable_indexes;
    }
    constants::peel_status_t status = constants::PEELED;
    if (nmax_count == MAX_CYCLE_COUNT) status = constants::INFINITE;
    else {
        for (std::size_t i = 0; i < buckets.size()and status == constants::PEELED; ++i) 
            if (buckets.at(i) != 0) status = constants::UNPEELABLE;
    }
    std::vector<record_t> toret(results.begin(), results.end());
    return std::make_pair(toret, status);
}

CLASS_HEADER
METHOD_HEADER&
METHOD_HEADER::operator-=(invertible_lookup_table const& other)
{
    if (not is_compatible(other)) throw std::invalid_argument("[operator-=] subtrahend is incompatible");
    for (std::size_t i = 0; i < nbuckets; ++i) 
    {
        auto my_bucket = bucket_idx_to_bucket_view(i);
        auto other_bucket = other.bucket_idx_to_bucket_view(i);
        *my_bucket.counter -= *other_bucket.counter;
        *my_bucket.length_sum ^= *other_bucket.length_sum;
        auto pld_ut_size = bit::round_up2(static_cast<std::size_t>(pld_bit_size), bit::size<UnderlyingType>()) / bit::size<UnderlyingType>();
        aligned_xor(my_bucket.payload_start, other_bucket.payload_start, pld_ut_size);
    }
    return *this;
}

CLASS_HEADER
METHOD_HEADER
METHOD_HEADER::generate_empty() const
{
    invertible_lookup_table toret;
    toret.pld_bit_size = pld_bit_size;
    toret.mseed = mseed;
    toret.nreps = nreps;
    toret.nbuckets = nbuckets;
    toret.bucket_byte_size = bucket_byte_size;
    toret.init();
    return toret;
}

CLASS_HEADER
template <class Visitor>
void 
METHOD_HEADER::visit(Visitor& visitor) const {
    visitor.visit(pld_bit_size);
    visitor.visit(nreps);
    visitor.visit(mseed);
    visitor.visit(nbuckets);
    visitor.visit(buckets);
}

CLASS_HEADER
template <class Visitor>
METHOD_HEADER 
METHOD_HEADER::load(Visitor& visitor) {
    invertible_lookup_table table;
    table.visit(visitor);
    assert(buckets.size() % nbuckets == 0);
    table.bucket_byte_size = buckets.size() / nbuckets;
    return table;
}

CLASS_HEADER
void 
METHOD_HEADER::init()
{
    buckets.resize(nbuckets * bucket_byte_size);
    clear();
}

CLASS_HEADER
void 
METHOD_HEADER::modify(std::vector<UnderlyingType> const& key, LengthType bit_len, bool addition)
{
    if (bit_len > pld_bit_size) throw std::invalid_argument("key's bit length (" + std::to_string(bit_len) + ") greater than allowed maximum (" + std::to_string(pld_bit_size) + ")");
    const std::size_t payload_ut_len = payload_len_as_number_of_underlying_type_integers();
    if (key.size() != payload_ut_len) throw std::runtime_error("key must be stored as an array of " + std::to_string(payload_ut_len) + " UnderlyingType integers");
    auto master_hash = hash::hash64::hash(reinterpret_cast<uint8_t const*>(key.data()), payload_ut_len * sizeof(UnderlyingType), mseed);
    for (std::size_t i = 0; i < nreps; ++i) {
        auto bucket_idx = hash::hash64::hash(master_hash, i) % nbuckets;
        auto bucket = bucket_idx_to_bucket_view(bucket_idx);
        if (addition) ++(*bucket.counter);
        else --(*bucket.counter);
        *bucket.length_sum ^= bit_len;
        aligned_xor(bucket.payload_start, key.data(), payload_ut_len);
    }
}

CLASS_HEADER
void  
METHOD_HEADER::aligned_xor(UnderlyingType* const payload_start, UnderlyingType const * const ptr, std::size_t ut_size)
{
    for (std::size_t i = 0; i < ut_size; ++i) payload_start[i] ^= ptr[i];
}

CLASS_HEADER
typename METHOD_HEADER::bucket_view_t  
METHOD_HEADER::bucket_idx_to_bucket_view(std::size_t bucket_idx) const
{
    assert(bucket_idx < nbuckets);
    auto byte_idx = bucket_idx * bucket_byte_size;
    assert(byte_idx < buckets.size());
    bucket_view_t toret;
    toret.counter = reinterpret_cast<CounterType*>(const_cast<uint8_t*>(&buckets[byte_idx]));
    byte_idx += sizeof(CounterType);
    toret.length_sum = reinterpret_cast<LengthType*>(const_cast<uint8_t*>(&buckets[byte_idx]));
    byte_idx += sizeof(LengthType);
    toret.payload_start = reinterpret_cast<UnderlyingType*>(const_cast<uint8_t*>(&buckets[byte_idx]));
    return toret;
}

CLASS_HEADER 
bool 
METHOD_HEADER::looks_pure(std::size_t bucket_idx, std::vector<std::size_t>& other_idxs) const
{
    auto bucket = bucket_idx_to_bucket_view(bucket_idx);
    const std::size_t payload_ut_len = payload_len_as_number_of_underlying_type_integers();
    auto master_hash = hash::hash64::hash(reinterpret_cast<uint8_t const*>(bucket.payload_start), payload_ut_len * sizeof(UnderlyingType), mseed);
    other_idxs.clear();
    bool ok = false;
    for (std::size_t i = 0; i < nreps; ++i) {
        auto check = hash::hash64::hash(master_hash, i) % nbuckets;
        if (bucket_idx == check) {
            auto bucket = bucket_idx_to_bucket_view(bucket_idx);
            if (*bucket.counter == 1 or *bucket.counter == -1) ok = true;
        } else {
            other_idxs.push_back(check);
        }
    }
    return ok;
}

CLASS_HEADER
bool 
METHOD_HEADER::is_compatible(invertible_lookup_table const& other) const noexcept
{
    return 
        pld_bit_size == other.pld_bit_size and 
        nreps == other.nreps and 
        mseed == other.mseed and 
        nbuckets == other.nbuckets and 
        bucket_byte_size == other.bucket_byte_size and 
        buckets.size() == other.buckets.size();
}

CLASS_HEADER 
template <class Visitor>
void 
METHOD_HEADER::visit(Visitor& visitor) 
{
    visitor.visit(pld_bit_size);
    visitor.visit(nreps);
    visitor.visit(mseed);
    visitor.visit(nbuckets);
    visitor.visit(buckets);
}

#undef METHOD_HEADER
#undef CLASS_HEADER

} // namespace bloom

#endif // INVERTIBLE_BLOOM_LOOKUP_TABLE_HPP

//----------------------------------------------------------------------------------------------------------

// CLASS_HEADER
// std::vector<typename METHOD_HEADER::record_t> 
// METHOD_HEADER::peel()
// {
//     std::array<std::size_t, (static_cast<std::size_t>(1) << bit::size<decltype(nreps)>())> npi; // new peelable indexes
//     std::size_t npi_size = 0;
//     std::set<std::size_t> peelable_indexes;
//     std::set<std::size_t> done;
//     for (std::size_t i = 0; i < size(); ++i) if (counters[i] == 1 or counters[i] == -1) peelable_indexes.insert(i);
//     std::vector<record_t> results;
//     while (peelable_indexes.size() > 0) { // no max passes threshold, we want to do better than 2 years ago
//         std::size_t bucket_idx;
//         {
//             auto itr = peelable_indexes.cbegin();
//             bucket_idx = *itr;
//             peelable_indexes.erase(itr);
//         }
//         get_at(bucket_idx); // now buffer contains the contents of the bucket
//         bool ok = false;
//         {
//             auto master_hash = hash::hash64::hash(reinterpret_cast<uint8_t const*>(buffer.data()), bit::round_up2(static_cast<std::size_t>(pld_bit_size), bit::size<UnderlyingType>()), mseed);
//             for (std::size_t i = 0; i < nreps; ++i) {
//                 auto check = hash::hash64::hash(master_hash, i) % chunk_size + i * chunk_size;
//                 if (bucket_idx == check) ok = true;
//                 npi[npi_size++] = check;
//             }
//         }
//         if (ok) {
//             done.insert(bucket_idx);
//             results.emplace_back(std::move(buffer), lengths.at(bucket_idx));
//             for (std::size_t i = 0; i < npi_size; ++i) {
//                 aligned_xor(npi[i], results.back().key.data(), results.back().key.size(), results.back().bit_len);
//                 if (done.find(npi[i]) == done.end() and (counters[npi[i]] == 1 or counters[npi[i]] == -1)) {
//                     peelable_indexes.insert(npi[i]);
//                 }
//             }
//         }
//     }
//     return results;
// }

// CLASS_HEADER
// std::pair<std::size_t, std::size_t> // index, shift 
// METHOD_HEADER::bucket_idx_to_ut_idx(std::size_t bucket_idx) const noexcept
// {
//     auto bit_idx = bucket_idx * pld_bit_size;
//     return std::make_pair(bit_idx / bit::size<UnderlyingType>(), bit_idx % bit::size<UnderlyingType>());
// }

/**
 * Retrieve a key from a bucket.
 * The resulting key is stored into buffer with its original alignment (right) preserved.  
 */
// CLASS_HEADER
// void 
// METHOD_HEADER::get_at(std::size_t bucket_idx) 
// {
//     // copy contents of bucket into buffer by reconstructing its original alignment
//     assert(counters.at(bucket_idx) == 1 or counters.at(bucket_idx) == -1);
//     auto [ut_idx, carry_length] = bucket_idx_to_ut_idx(bucket_idx);
//     const auto fitting_length = (bit::size<UnderlyingType>() - carry_length);
//     const auto mask = (UnderlyingType(1) << fitting_length) - 1;
//     buffer.clear();

//     auto ut_end_idx = ut_idx + (pld_bit_size + carry_length) / bit::size<UnderlyingType>();
//     for (std::size_t i = ut_idx; i < ut_end_idx; ++i) {
//         buffer.push_back((buckets.at(i) & mask) | (buckets.at(i + 1) & ~mask));
//     }
//     auto back_shift = bit::round_up2<std::size_t>(pld_bit_size + carry_length, bit::size<UnderlyingType>()) - (pld_bit_size + carry_length);
//     buffer.push_back(buckets.at(ut_end_idx) >> (bit::size<UnderlyingType>() - back_shift));
// }

// CLASS_HEADER
// void 
// METHOD_HEADER::modify(UnderlyingType const * const ptr, LengthType bit_len, bool addition)
// {
//     buffer.reserve(pld_bit_size / bit::size<UnderlyingType>() + 1);
//     const auto ptr_ut_len = bit::round_up2(static_cast<std::size_t>(bit_len), bit::size<UnderlyingType>()) / sizeof(UnderlyingType);
//     auto master_hash = hash::hash64::hash(reinterpret_cast<uint8_t const*>(ptr), ptr_ut_len, mseed);
//     for (std::size_t i = 0; i < nreps; ++i) {
//         auto bucket_idx = hash::hash64::hash(master_hash, i) % chunk_size + i * chunk_size;
//         aligned_xor(bucket_idx, ptr, ptr_ut_len, bit_len);
//         if (addition) ++counters[bucket_idx];
//         else --counters[bucket_idx];
//     }
// }

/**
 * ptr is a pointer to a sequence of Underlying type right-aligned:aligned
 * example with UnderlyingType = uint8_t and a value of 11 bits, numbers are bit indexes
 * [7 6 5 4 3 2 1 0], [* * * * * 10 9 8]
 */
// CLASS_HEADER
// void 
// METHOD_HEADER::aligned_xor(std::size_t bucket_idx, UnderlyingType const * const ptr, std::size_t ptr_ut_len, LengthType bit_len)
// {
//     assert(bit_len <= pld_bit_size);
//     auto [ut_idx, carry_length] = bucket_idx_to_ut_idx(bucket_idx);
//     const auto fitting_length = (bit::size<UnderlyingType>() - carry_length);
//     const auto mask = (UnderlyingType(1) << fitting_length) - 1;
//     buffer.clear();

//     UnderlyingType carry = 0;
//     for (std::size_t i = 0; i < ptr_ut_len; ++i) { 
//         buffer.push_back(carry | (ptr[i] & mask)); // this actually breaks input bit-order but it's faster to compute
//         carry = ptr[i] & ~mask;
//         bit_len -= fitting_length;
//         if (i != 0) bit_len -= carry_length;
//     }
//     // the last carry needs special handling since its length is the number of remaining bits to be packed
//     buffer.push_back(carry << (bit::size<UnderlyingType>() - bit_len));

//     for (std::size_t i = 0; i < buffer.size(); ++i) { // TODO modify buckets in-place after debug (remove buffer usage)
//         buckets[ut_idx] ^= buffer[i];
//         ++ut_idx;
//     }
// }

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