#ifndef RANK_SELECT_QUOTIENT_FILTER_HPP
#define RANK_SELECT_QUOTIENT_FILTER_HPP

#include "packed_vector.hpp"

namespace membership {
namespace approximate {

#define CLASS_HEADER template <class UnderlyingType>
#define METHOD_HEADER rank_select_quotient_filter<UnderlyingType>

CLASS_HEADER
class rank_select_quotient_filter
{
    public:
        using key_type = UnderlyingType;
        using value_type = key_type; // it is a set, so same as key_type
        using remainder_pack_t = UnderlyingType;

        rank_select_quotient_filter(uint8_t hash_bit_size, uint8_t remainder_bit_size);
        std::size_t capacity() const noexcept;
        std::size_t size() const noexcept;
        bool empty() const noexcept;
        // std::size_t max_size() const;
        // std::size_t count(const key_type hval) const;
        // const_iterator find(const key_type hval) const;
        bool contains(const key_type& hval) const;
        // std::pair<const_iterator, const_iterator> equal_range(const key_type hval) const;
        float load_factor() const;
        float max_load_factor() const;

        void max_load_factor(float ml);
        void rehash(std::size_t n);
        void merge(METHOD_HEADER& source);
        void insert(const key_type& hval); // switch to return type of std::pair<iterator, bool> 
        void erase(const key_type hval);
        void clear();

        // template <typename T>
        // std::pair<iterator, bool> insert(T&& value);

        // template <class Iterator>
        // void insert(Iterator start, Iterator end);

        // template <typename T>
        // void erase(T&& value);

        // template <typename T>
        // std::size_t count(const T& value) const;

    private:
        static const std::size_t remainders_per_block = bit::size<remainder_pack_t>();
        struct block_t
        {
            uint8_t offset;
            UnderlyingType occupieds;
            UnderlyingType runends;
            remainder_pack_t* remainders;
        };

        std::size_t nelems;
        uint8_t hash_bitwidth;
        uint8_t remainder_bitwidth; // can't be const since it can change when we rehash/merge
        std::vector<uint8_t> blocks_data; // can't use an helper class because parameters are dynamic
        uint8_t quotient_bitwidth() const noexcept;

        std::tuple<key_type, key_type> get_quotient_and_remainder(key_type hval) const;
        void insert_in_empty_slot(std::size_t idx, key_type remainder);
        void shift_slots_between(std::size_t i, std::size_t j);
        void update_metadata();

        std::size_t find_runend(std::size_t idx) const;
        void set_occupieds_at(std::size_t idx);
        void clear_occupieds_at(std::size_t idx);
        bool get_occupieds_at(std::size_t idx) const noexcept;
        void set_runends_at(std::size_t idx);
        void clear_runends_at(std::size_t idx);
        bool get_runends_at(std::size_t idx) const noexcept;
        
        std::size_t blocks_size() const noexcept;
        std::size_t block_byte_size() const noexcept;
        std::tuple<block_t&, std::size_t> index_to_block_coordinates(std::size_t idx);
        block_t& block_at(std::size_t idx);
        std::size_t get_remainder_at(block_t& block, std::size_t idx);
        void set_remainder_at(block_t& block, std::size_t idx, std::size_t val);
        std::vector<UnderlyingType> shift_remainders(block_t& block, long long shift); // shift as number of positions, not as bitwidth
        std::tuple<std::size_t, long long> index_to_remainder_coordinates(std::size_t remainder_index) const noexcept;

    private: // helpers
        struct block_header_t // helper, not actually used
        {
            uint8_t offset;
            UnderlyingType occupieds;
            UnderlyingType runends;
        }; 
};

template <typename HashFunction>
using RSQF = rank_select_quotient_filter<HashFunction>;

CLASS_HEADER
METHOD_HEADER::rank_select_quotient_filter(uint8_t hash_bit_size, uint8_t remainder_bit_size)
    : nelems(0), hash_bitwidth(hash_bit_size), remainder_bitwidth(remainder_bit_size)
{
    if (hash_bitwidth < remainder_bitwidth) throw std::length_error("[rank_select_qotient_filter] remainder size greater than hash type size");
    const auto blocks_byte_size = block_byte_size() * blocks_size();
    blocks_data.resize(blocks_byte_size);
}

CLASS_HEADER
std::size_t
METHOD_HEADER::capacity() const noexcept
{
    return 1ULL << quotient_bitwidth();
}

CLASS_HEADER
std::size_t
METHOD_HEADER::size() const noexcept
{
    return nelems;
}

CLASS_HEADER
bool
METHOD_HEADER::empty() const noexcept
{
    return size() == 0;
}

CLASS_HEADER
bool
METHOD_HEADER::contains(const key_type& hval) const
{
    auto [q, r] = get_quotient_and_remainder(hval);
    
    if (get_occupieds(q)) {
        auto s = find_runend(q);
        auto [block, local_shift] = index_to_block_coordinates(s);
        do {
            if (get_remainder_at(block, local_shift) == r) return true;
            --s;
            if (local_shift == 0) {
                std::tie(block, local_shift) = index_to_block_coordinates(s);
            } else --local_shift;
        } while(s > q and not get_runends(s));
    }
    return false;
}

CLASS_HEADER
void
METHOD_HEADER::insert(const key_type& hval)
{
    auto find_first_unused_slot = [this](std::size_t idx) {
        auto s = find_runend(idx);
        while (idx <= s) {
            idx = s + 1;
            s = find_runend(idx);
        }
        return idx;
    };
    auto [q, rem] = get_quotient_and_reminder(hval);
    auto s = find_runend(q);
    if (q > s) {
        insert_in_empty_slot(q, rem);
        set_runends(q);
        update_metadata();
    } else {
        ++s;
        auto n = find_first_unused_slot(s);
        shift_slots_between(s, n);
        insert_in_empty_slot(s, rem);
        set_runends(s);
        if (get_occupieds(q)) clear_runends(s-1);
        update_metadata();
    }
    set_occupieds(q);
}

CLASS_HEADER
uint8_t 
METHOD_HEADER::quotient_bitwidth() const noexcept
{
    return hash_bitwidth - remainder_bitwidth;
}

CLASS_HEADER
std::tuple<typename METHOD_HEADER::key_type, typename METHOD_HEADER::key_type>
METHOD_HEADER::get_quotient_and_remainder(key_type hval) const
{
    const key_type remainder = hval & ((static_cast<key_type>(1) << remainder_bitwidth) - 1);
    const key_type quotient = hval >> remainder_bitwidth;
    if (quotient >= blocks_data.size()) throw std::length_error("[get_quotient_and_remainder] quotient out of bounds");
    return std::make_tuple(quotient, remainder);
}

CLASS_HEADER
void
METHOD_HEADER::set_occupieds_at(std::size_t idx)
{
    auto [block, local_idx] = index_to_block_coordinates(idx);
    block.occupieds |= static_cast<UnderlyingType>(1) << local_idx;
}

CLASS_HEADER
void
METHOD_HEADER::clear_occupieds_at(std::size_t idx)
{
    auto [block, local_idx] = index_to_block_coordinates(idx);
    block.occupieds &= ~(static_cast<UnderlyingType>(1) << local_idx);
}

CLASS_HEADER
bool
METHOD_HEADER::get_occupieds_at(std::size_t idx) const noexcept
{
    auto [block, local_idx] = index_to_block_coordinates(idx);
    return (block.occupieds & (static_cast<UnderlyingType>(1) << local_idx)) != 0;
}

CLASS_HEADER
void
METHOD_HEADER::set_runends_at(std::size_t idx)
{
    auto [block, local_idx] = index_to_block_coordinates(idx);
    block.runends |= static_cast<UnderlyingType>(1) << local_idx;
}

CLASS_HEADER
void
METHOD_HEADER::clear_runends_at(std::size_t idx)
{
    auto [block, local_idx] = index_to_block_coordinates(idx);
    block.runends &= ~(static_cast<UnderlyingType>(1) << local_idx);
}

CLASS_HEADER
bool
METHOD_HEADER::get_runends_at(std::size_t idx) const noexcept
{
    auto [block, local_idx] = index_to_block_coordinates(idx);
    return (block.runends & (static_cast<UnderlyingType>(1) << local_idx)) != 0;
}

CLASS_HEADER
void
METHOD_HEADER::insert_in_empty_slot(std::size_t idx, key_type remainder)
{
    auto [block, local_idx] = index_to_block_coordinates(idx);
    set_remainder_at(block, local_idx, remainder);
}

CLASS_HEADER
void 
METHOD_HEADER::shift_slots_between(std::size_t i, std::size_t j)
{
    // TODO
}

CLASS_HEADER
void 
METHOD_HEADER::update_metadata()
{
    // TODO
}

CLASS_HEADER
std::size_t 
METHOD_HEADER::find_runend(std::size_t idx) const
{
    auto [block, local_idx] = index_to_block_coordinates(idx);
    auto local_rank = bit::rank1(block.occupieds, local_idx);
    auto select_idx_start = idx - (idx % remainders_per_block) + block.offset;

    // find the local_rank-th set bit in runends from position select_idx_start
    auto [bitr, blidx] = index_to_block_coordinates(select_idx_start);
    auto pcnt = bit::popcount(bitr.runends >> blidx);
    while (pcnt < local_rank) {
        local_rank -= pcnt;
        select_idx_start += remainders_per_block - blidx;
        std::tie(bitr, blidx) = index_to_block_coordinates(select_idx_start);
        pcnt = bit::popcount(bitr.runends >> blidx);
    }
    return select_idx_start + bit::select1(bitr.runends >> blidx);
}

CLASS_HEADER
std::size_t
METHOD_HEADER::blocks_size() const noexcept
{
    return capacity() / remainders_per_block;
}

CLASS_HEADER
std::size_t 
METHOD_HEADER::block_byte_size() const noexcept
{
    return (remainders_per_block * remainder_bitwidth) + sizeof(block_header_t);
}

CLASS_HEADER
typename METHOD_HEADER::block_t&
METHOD_HEADER::block_at(std::size_t idx)
{
    auto bidx = block_byte_size() * idx;
    if (bidx >= blocks_data.size()) throw std::out_of_range("Trying to access out of bounds block");
    return *reinterpret_cast<block_t*>(&blocks_data[block_byte_size() * idx]);
}

CLASS_HEADER
std::tuple<typename METHOD_HEADER::block_t&, std::size_t> 
METHOD_HEADER::index_to_block_coordinates(std::size_t idx)
{
    return std::make_tuple(block_at(idx / remainders_per_block), idx % remainders_per_block);
}

CLASS_HEADER
std::tuple<std::size_t, long long> 
METHOD_HEADER::index_to_remainder_coordinates(std::size_t remainder_index) const noexcept
{
    std::size_t ut_idx = (remainder_index * remainder_bitwidth) / remainders_per_block;
    long long ut_shift = remainders_per_block - remainder_bitwidth - ((remainder_index * remainder_bitwidth) % remainders_per_block);
    return std::make_tuple(ut_idx, ut_shift);
}

CLASS_HEADER
std::size_t
METHOD_HEADER::get_remainder_at(block_t& block, std::size_t position)
{
    if (position >= remainders_per_block) throw std::out_of_range("Trying to access out of bounds remainder in block");
    auto [idx, shift] = index_to_remainder_coordinates(position);
    if (shift < 0) { // crossing border
        auto buffer = block.remainders[idx];
        remainder_pack_t mask_shift = remainder_bitwidth + shift;
        buffer &= (remainder_pack_t(1) << mask_shift) - 1;
        buffer <<= -shift;
        buffer |= block.remainders[idx + 1] >> (remainders_per_block + shift);
        return static_cast<std::size_t>(buffer);
    } else { // perfect fit
        auto buffer = block.remainders[idx] >> shift;
        if (remainder_bitwidth != remainders_per_block) { // mask if it does not exactly fits
            buffer &= ((static_cast<remainder_pack_t>(1) << remainder_bitwidth) - 1);
        }
        return static_cast<std::size_t>(buffer);
    }
    // should never reach here
}

CLASS_HEADER
void
METHOD_HEADER::set_remainder_at(block_t& block, std::size_t position, std::size_t val)
{
    assert(bit::msbll(val) < remainder_bitwidth());
    auto [idx, shift] = index_to_remainder_coordinates(position);
    if (shift < 0) { // crossing border
        remainder_pack_t mask_shift = remainder_bitwidth() + shift;
        block.remainders[idx] &= ~((remainder_pack_t(1) << mask_shift) - 1);
        block.remainders[idx] |= static_cast<remainder_pack_t>(val >> -shift);
        block.remainders[idx + 1] &= ((remainder_pack_t(1) << (remainders_per_block - remainder_bitwidth)) - 1);
        block.remainders[idx + 1] |= val << (remainders_per_block - remainder_bitwidth);
    } else { // perfect fit
        remainder_pack_t buffer = val << shift;
        buffer |= block.remainders[idx] & ((static_cast<remainder_pack_t>(1) << shift) - 1); 
        buffer |= ~(static_cast<remainder_pack_t>(1) << ((remainder_bitwidth + shift) - 1));
        block.remainders[idx] = buffer;
    }
}

CLASS_HEADER
std::vector<UnderlyingType>
METHOD_HEADER::shift_remainders(block_t& block, long long shift)
{
    // shift remainders also across neighboring blocks
    // TODO
}

#undef METHOD_HEADER
#undef CLASS_HEADER

} // namespace approximate
} // namespace membership

#endif // RANK_SELECT_QUOTIENT_FILTER_HPP