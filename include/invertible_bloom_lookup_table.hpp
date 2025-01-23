#ifndef INVERTIBLE_BLOOM_LOOKUP_TABLE_HPP
#define INVERTIBLE_BLOOM_LOOKUP_TABLE_HPP

#include <cstddef>
#include <cstdint>
#include <array>
#include <vector>

namespace bloom {

class invertible_lookup_table
{
    public:
        invertible_lookup_table(
            uint8_t counter_bit_size, 
            uint8_t payload_size, 
            uint8_t repetitions, 
            float epsilon, 
            std::size_t expected_number_of_differences,
            uint64_t seed
        );

        template <class Iterator>
        void insert(Iterator start, Iterator end);

        template <class Iterator>
        void remove(Iterator start, Iterator end);

        void clear();
        std::vector<std::string> peel();
        std::size_t size() const noexcept;
        uint8_t const* data() const noexcept;
        invertible_lookup_table& operator-=(invertible_lookup_table const& other);

    private:
        friend invertible_lookup_table operator-(invertible_lookup_table const& first, invertible_lookup_table const& second);
        std::size_t get_count(std::size_t bucket_idx) const;
        static const std::array<float, 8> ck_table;
        uint8_t cnt_size;
        uint8_t pld_size;
        uint8_t nreps;
        float eps;
        std::size_t mseed;
        std::size_t chunk_size;
        std::size_t bucket_byte_size;
        std::vector<uint8_t> buckets;
};

template <class Iterator>
void invertible_lookup_table::insert(Iterator start, Iterator end)
{
    //
}

template <class Iterator>
void invertible_lookup_table::remove(Iterator start, Iterator end)
{
    //
}

} // namespace bloom

#endif // INVERTIBLE_BLOOM_LOOKUP_TABLE_HPP