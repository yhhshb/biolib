#ifndef INVERTIBLE_BLOOM_LOOKUP_TABLE_HPP
#define INVERTIBLE_BLOOM_LOOKUP_TABLE_HPP

#include <cstddef>
#include <cstdint>
#include <cmath>
#include <string>
#include <unordered_set>
#include <ostream>
#include "bit_operations.hpp"
#include "hash.hpp"

#include "../bundled/prettyprint.hpp"

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
            friend bool operator==(record_t const& a, record_t const& b) {return a.key == b.key and a.bit_len == b.bit_len;} // and a.sign == b.sign;}
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
        std::vector<UnderlyingType> get_key_buffer() const;
        void insert(std::vector<UnderlyingType> const& key, std::size_t bit_len) {modify(key, bit_len, true);}
        void remove(std::vector<UnderlyingType> const& key, std::size_t bit_len) {modify(key, bit_len, false);}
        void clear() noexcept;
        bool empty() const noexcept;
        std::pair<std::vector<record_t>, constants::peel_status_t> peel();
        std::size_t size() const noexcept {return nbuckets;}
        invertible_lookup_table& operator-=(invertible_lookup_table const& other);
        invertible_lookup_table generate_empty() const;

        template <class Visitor> void visit(Visitor& visitor) const;
        template <class Visitor> invertible_lookup_table load(Visitor& visitor);

        std::vector<std::size_t> payload_to_bucket_indexes(UnderlyingType const * const key) const noexcept;
        std::vector<std::size_t> bucket_idx_to_bucket_indexes(const std::size_t bucket_idx) const noexcept;

    private:
        struct record_hash_t { // hash function for record_t which only depends on "key" field
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

        friend bool operator==(invertible_lookup_table const& a, invertible_lookup_table const& b)
        {
            if (not a.is_compatible(b)) return false;
            for (std::size_t i = 0; i < a.buckets.size(); ++i) {
                if (a.buckets.at(i) != b.buckets.at(i)) return false;
            }
            return true;
        }
        friend bool operator!=(invertible_lookup_table const& a, invertible_lookup_table const& b) {return not a == b;}

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
    nbuckets = static_cast<std::size_t>(std::ceil(expected_number_of_differences * (constants::ilt_scaling_table[nreps] + static_cast<double>(epsilon))));
    std::size_t counter_byte_size = sizeof(CounterType);
    std::size_t length_byte_size = sizeof(LengthType);
    std::size_t payload_byte_size = bit::round_up2(static_cast<std::size_t>(pld_bit_size), bit::size<UnderlyingType>()) / bit::size<uint8_t>();
    bucket_byte_size = counter_byte_size + length_byte_size + payload_byte_size;
    init();
}

CLASS_HEADER
void 
METHOD_HEADER::clear() noexcept
{
    for (std::size_t i = 0; i < buckets.size(); ++i) buckets[i] = 0;
}

CLASS_HEADER
bool 
METHOD_HEADER::empty() const noexcept
{
    for (std::size_t i = 0; i < buckets.size(); ++i) {
        if (buckets[i] != 0) return false;
    }
    return true;
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
    // std::cerr << *this << "\n";
    // std::cerr << "Starting points: " << peelable_indexes << "\n";
    std::unordered_set<record_t, record_hash_t> results;
    std::vector<std::size_t> next_peelable_indexes;
    std::vector<std::size_t> dummy;
    record_t record;
    const std::size_t payload_ut_len = payload_len_as_number_of_underlying_type_integers();
    record.key.resize(payload_ut_len);
    std::size_t max_output_size = 1;
    std::size_t nmax_count = 0;
    while (not peelable_indexes.empty() and nmax_count != MAX_CYCLE_COUNT) {
        next_peelable_indexes.clear();
        for (auto bucket_idx : peelable_indexes) {
            // std::cerr << *this << "\n";
            // std::cerr << "\tlooking at bucket idx: " << bucket_idx << "\n";
            if (looks_pure(bucket_idx, other_idxs)) {
                auto bucket = bucket_idx_to_bucket_view(bucket_idx);
                record.bit_len = *bucket.length_sum;
                for (std::size_t i = 0; i < payload_ut_len; ++i) { // extract payload
                    record.key[i] = bucket.payload_start[i];
                }
                if (*bucket.counter == 1) { // update buckets
                    remove(record.key, record.bit_len);
                    record.sign = true;
                } else if (*bucket.counter == -1) {
                    insert(record.key, record.bit_len);
                    record.sign = false;
                } else {
                    assert(false);
                }
                // std::vector<std::size_t> printable_key(record.key.cbegin(), record.key.cend());
                // std::cerr << "\t>>> Extracted key: " << (record.sign ? "+" : "-") << printable_key << "\n";
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
                // std::cerr << "\tOther indexes: " << other_idxs << "\n";
                for (auto idx : other_idxs) {
                    if (looks_pure(idx, dummy)) {
                        next_peelable_indexes.push_back(idx);
                    }
                }
            }
        }
        // std::cerr << "next peelable indexes: " << next_peelable_indexes << "\n\n";
        // peelable_indexes.clear();
        // for (auto ritr = next_peelable_indexes.crbegin(); ritr != next_peelable_indexes.crend(); ++ritr) peelable_indexes.push_back(*ritr);
        peelable_indexes = next_peelable_indexes;
    }
    constants::peel_status_t status = constants::PEELED;
    if (nmax_count == MAX_CYCLE_COUNT) status = constants::INFINITE;
    else if (not empty()) status = constants::UNPEELABLE;
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
std::vector<std::size_t> 
METHOD_HEADER::payload_to_bucket_indexes(UnderlyingType const * const payload) const noexcept
{
    const std::size_t payload_ut_len = payload_len_as_number_of_underlying_type_integers();
    auto master_hash = hash::hash64::hash(reinterpret_cast<uint8_t const*>(payload), payload_ut_len * sizeof(UnderlyingType), mseed);
    std::vector<std::size_t> toret;
    for (std::size_t i = 0; i < nreps; ++i) toret.push_back(hash::hash64::hash(master_hash, i) % nbuckets);
    return toret;
}

CLASS_HEADER
std::vector<std::size_t> 
METHOD_HEADER::bucket_idx_to_bucket_indexes(const std::size_t bucket_idx) const noexcept
{
    const auto payload_ut_len = payload_len_as_number_of_underlying_type_integers();
    auto bucket = bucket_idx_to_bucket_view(bucket_idx);
    auto master_hash = hash::hash64::hash(reinterpret_cast<uint8_t const*>(bucket.payload_start), payload_ut_len * sizeof(UnderlyingType), mseed);
    std::vector<std::size_t> toret;
    for (std::size_t i = 0; i < nreps; ++i) toret.push_back(hash::hash64::hash(master_hash, i) % nbuckets);
    return toret;
}

CLASS_HEADER
void 
METHOD_HEADER::init()
{
    buckets.resize(nbuckets * bucket_byte_size);
    clear();
}

CLASS_HEADER
std::vector<UnderlyingType> 
METHOD_HEADER::get_key_buffer() const
{
    const std::size_t payload_ut_len = payload_len_as_number_of_underlying_type_integers();
    return std::vector<UnderlyingType>(payload_ut_len);
}

CLASS_HEADER
void 
METHOD_HEADER::modify(std::vector<UnderlyingType> const& key, LengthType bit_len, bool addition)
{
    if (bit_len > pld_bit_size) throw std::invalid_argument("key's bit length (" + std::to_string(bit_len) + ") greater than allowed maximum (" + std::to_string(pld_bit_size) + ")");
    const std::size_t payload_ut_len = payload_len_as_number_of_underlying_type_integers();
    if (key.size() != payload_ut_len) throw std::runtime_error("key must be of fixed size " + std::to_string(payload_ut_len));

    auto bucket_indexes = payload_to_bucket_indexes(key.data());
    for (auto bucket_idx : bucket_indexes) {
        auto bucket = bucket_idx_to_bucket_view(bucket_idx);
        if (addition) ++(*bucket.counter);
        else --(*bucket.counter);
        *bucket.length_sum ^= bit_len;
        aligned_xor(bucket.payload_start, key.data(), key.size());
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
    auto checks = payload_to_bucket_indexes(bucket.payload_start);
    other_idxs.clear();
    bool ok = false;
    for (auto check : checks) {
        if (bucket_idx == check) {
            if (((*bucket.counter == 1) or (*bucket.counter == -1)) and *bucket.length_sum <= pld_bit_size) ok = true;
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
