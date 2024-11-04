#ifndef RANK_SELECT_QUOTIENT_FILTER_HPP
#define RANK_SELECT_QUOTIENT_FILTER_HPP

#include "bit_operations.hpp"
#include "toolbox.hpp"

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

        class const_iterator
        {
            public:
                using iterator_category = std::forward_iterator_tag;
                using difference_type   = std::ptrdiff_t;
                using value_type        = key_type;
                using pointer           = value_type*;
                using reference         = value_type&;

                const_iterator(rank_select_quotient_filter const* parent, bool is_start);
                value_type operator*() const;
                const_iterator const& operator++() noexcept;
                const_iterator operator++(int) noexcept;

            private:
                rank_select_quotient_filter const* parent_filter;
                std::size_t idx;
                std::size_t runend_idx;

                void find_next_run();

                friend bool operator==(const_iterator const& a, const_iterator const& b) 
                {
                    bool same_filter = a.parent_filter == b.parent_filter;
                    bool same_start = a.idx == b.idx;
                    return same_filter and same_start;
                };
                friend bool operator!=(const_iterator const& a, const_iterator const& b) {return not (a == b);};
        };

        rank_select_quotient_filter(uint8_t hash_bit_size, uint8_t remainder_bit_size);
        uint8_t quotient_bitwidth() const noexcept;
        uint8_t remainder_bitwidth() const noexcept;
        uint8_t hash_bitwidth() const noexcept;
        std::size_t capacity() const noexcept;
        std::size_t size() const noexcept;
        bool empty() const noexcept;
        // std::size_t max_size() const;
        // std::size_t count(const key_type hval) const;
        // const_iterator find(const key_type hval) const;
        bool contains(const key_type& hval) const;
        // std::pair<const_iterator, const_iterator> equal_range(const key_type hval) const;
        float load_factor() const noexcept;
        float max_load_factor() const noexcept;
        const_iterator cbegin() const;
        const_iterator cend() const;

        void max_load_factor(float ml);
        void rehash(std::size_t n);
        void merge(METHOD_HEADER& source);
        void insert(key_type const& hval); // switch to return type of std::pair<iterator, bool> 
        void erase(key_type const& hval);
        void clear();
        void swap(METHOD_HEADER& other);

        void print_metadata() const noexcept;
        void print_remainders() const noexcept;

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
        static const UnderlyingType last_bit_mask = static_cast<UnderlyingType>(1) << (bit::size<UnderlyingType>() - 1); // create a mask to access the last bit a UnderlyingType variable
        struct block_t
        {
            uint8_t* offset;
            UnderlyingType* occupieds;
            UnderlyingType* runends;
            remainder_pack_t* remainders;
        };

        float m_load_factor;
        std::size_t m_size;
        uint8_t m_hash_bitwidth;
        uint8_t m_remainder_bitwidth; // can't be const since it can change when we rehash/merge
        std::vector<uint8_t> blocks_data; // can't use an helper class because parameters are dynamic

        std::tuple<key_type, key_type> get_quotient_and_remainder(key_type hval) const;
        void insert_in_empty_slot(std::size_t idx, key_type remainder);
        std::pair<key_type, bool> right_shift_slots_between(std::size_t start, std::size_t stop, std::pair<key_type, bool> carry);

        std::size_t find_runend(std::size_t idx) const;
        void set_occupieds_at(std::size_t idx);
        void clear_occupieds_at(std::size_t idx);
        bool get_occupieds_at(std::size_t idx) const noexcept;
        void set_runends_at(std::size_t idx);
        void clear_runends_at(std::size_t idx);
        bool get_runends_at(std::size_t idx) const noexcept;
        std::size_t get_remainder_at(std::size_t idx) const;
        
        std::size_t blocks_size() const noexcept;
        std::size_t block_byte_size() const noexcept;
        std::tuple<std::size_t, std::size_t> index_to_block_indexes(std::size_t idx) const noexcept;
        std::tuple<block_t, std::size_t> index_to_block_coordinates(std::size_t idx) const noexcept;
        // std::tuple<block_t&, std::size_t> index_to_block_coordinates(std::size_t idx) noexcept;
        // const_block_t block_at(std::size_t idx) const;
        block_t block_at(std::size_t idx) const;
        std::size_t get_remainder_at(block_t const& block, std::size_t idx) const;
        void set_remainder_at(block_t& block, std::size_t idx, std::size_t val);
        std::size_t right_shift_remainders(block_t& block, std::size_t start, std::size_t stop, std::size_t val);
        std::tuple<std::size_t, long long> index_to_remainder_coordinates(std::size_t remainder_index) const noexcept;
};

template <typename HashFunction>
using RSQF = rank_select_quotient_filter<HashFunction>;

CLASS_HEADER
METHOD_HEADER::rank_select_quotient_filter(uint8_t hash_bit_size, uint8_t remainder_bit_size)
    : m_load_factor(0.95), m_size(0), m_hash_bitwidth(hash_bit_size), m_remainder_bitwidth(remainder_bit_size)
{
    if (m_hash_bitwidth < m_remainder_bitwidth) throw std::length_error("[rank_select_qotient_filter] remainder size greater than hash type size");
    const auto blocks_byte_size = block_byte_size() * blocks_size();
    blocks_data.resize(blocks_byte_size);
    // fprintf(stderr, "blocks_size = %llu, block_byte_size = %llu, blocks_data.size = %llu\n", 
    //     blocks_size(), 
    //     block_byte_size(), 
    //     blocks_data.size()
    // );
}

CLASS_HEADER
uint8_t 
METHOD_HEADER::quotient_bitwidth() const noexcept
{
    return m_hash_bitwidth - m_remainder_bitwidth;
}

CLASS_HEADER
uint8_t 
METHOD_HEADER::remainder_bitwidth() const noexcept
{
    return m_remainder_bitwidth;
}

CLASS_HEADER
uint8_t 
METHOD_HEADER::hash_bitwidth() const noexcept
{
    return m_hash_bitwidth;
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
    return m_size;
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
    // fprintf(stderr, "-------------------------------------------------------\n");
    auto [q, r] = get_quotient_and_remainder(hval);
    // fprintf(stderr, "q = %llu, r = %llu\n", q, r);
    if (get_occupieds_at(q)) {
        auto s = find_runend(q);
        // fprintf(stderr, "runend index is %llu\n", s);
        
        if (s == 0) s = capacity() - 1;
        else --s;
        auto end = (q == 0) ? capacity() - 1 : q - 1;

        auto [block_idx, local_shift] = index_to_block_indexes(s);
        auto block = block_at(block_idx);
        do {
            auto rem = get_remainder_at(block, local_shift);
            // fprintf(stderr, "position %llu contains remainder %llu\n", s, rem);
            if (rem == r) {
                // fprintf(stderr, "*******************************************************\n");
                return true;
            }
            if (s == 0) s = capacity() - 1;
            else --s;
            if (local_shift == 0) {
                std::tie(block_idx, local_shift) = index_to_block_indexes(s);
                block = block_at(block_idx);
            } else {
                --local_shift;
            }
        } while(s != end and not get_runends_at(s));
    }
    // fprintf(stderr, "*******************************************************\n");
    return false;
}

CLASS_HEADER
float 
METHOD_HEADER::load_factor() const noexcept
{
    return static_cast<float>(size()) / capacity();
}

CLASS_HEADER
float 
METHOD_HEADER::max_load_factor() const noexcept
{
    return m_load_factor;
}

CLASS_HEADER
typename METHOD_HEADER::const_iterator 
METHOD_HEADER::cbegin() const
{
    return const_iterator(this, true);
}

CLASS_HEADER
typename METHOD_HEADER::const_iterator
METHOD_HEADER::cend() const
{
    return const_iterator(this, false);
}

CLASS_HEADER
void 
METHOD_HEADER::max_load_factor(float max_load_factor)
{
    if (max_load_factor > 1 or max_load_factor < 0) throw std::invalid_argument("load factor must be between 0 and 1");
    m_load_factor = max_load_factor;
}

CLASS_HEADER
void 
METHOD_HEADER::rehash(std::size_t n)
{
    auto pos = bit::msbll(n);
    if ((static_cast<std::size_t>(1) << pos) ^ n) n = static_cast<std::size_t>(1) << (pos + 1);
    if (quotient_bitwidth() == hash_bitwidth()) throw std::runtime_error("[rehash] unable to increase quotient size, already using all bits");
    rank_select_quotient_filter<UnderlyingType> qf_new(m_hash_bitwidth, m_remainder_bitwidth - 1);
    for (auto itr = cbegin(); itr != cend(); ++itr) {
        qf_new.insert(*itr);
    }
    swap(qf_new);
}

CLASS_HEADER
void 
METHOD_HEADER::merge(METHOD_HEADER& source)
{
    if (static_cast<float>(size() + source.size()) / capacity() > max_load_factor()) rehash(2 * capacity());
    for (auto itr = source.cbegin(); itr != source.cend(); ++itr) {
        insert(*itr);
    }
}

CLASS_HEADER
void
METHOD_HEADER::insert(key_type const& hval)
{
    auto find_first_unused_slot = [this](std::size_t idx) {
        auto starting_idx = idx;
        auto s = find_runend(idx);
        while (idx != s) {
            idx = s; // + 1
            s = find_runend(idx);
            fprintf(stderr, "[ffus loop] idx: %llu, s: %llu\n", idx, s);
            if (idx == starting_idx) throw std::runtime_error("[find_first_unused_slot] infinite loop");
        }
        return idx;
    };

    fprintf(stderr, "-------------------------------------------------------\n");
    if (static_cast<float>(m_size + 1) / capacity() > max_load_factor()) {
        fprintf(stderr, "REHASHING\n");
        rehash(2 * capacity());
    }
    auto [q, rem] = get_quotient_and_remainder(hval);
    auto s = find_runend(q);
    fprintf(stderr, "inserting %llu in position %llu\n", rem, q);
    fprintf(stderr, "runend of q(%llu) is %llu\n", q, s);
    if (s != q) {
        auto n = find_first_unused_slot(s);
        fprintf(stderr, "collision, first unused slot from %llu is %llu\n", s, n);
        assert(n < capacity());
        assert(not get_occupieds_at(n));
        assert(not get_runends_at(n));
        assert(not get_remainder_at(n));
        {
            std::pair<key_type, bool> ctx = {rem, true};
            if (s <= n) { // no circularity needed
                // fprintf(stderr, "no circularity\n");
                ctx = right_shift_slots_between(s, n, ctx);
            } else if (n < s){ // circular shift
                // fprintf(stderr, "circularity\n");
                ctx = right_shift_slots_between(s, capacity() - 1, ctx);
                ctx = right_shift_slots_between(0, n, ctx);
            }
            assert(ctx.first == 0);
        }
        // insert_in_empty_slot(s, rem); // this is done together with the shift at the previous step, for better cache locality
        // set_runends_at(s); same as before, to avoid going back to position s, its bit is set during the shift
        // update_metadata(); same for metadata (offsets), update them while shifting
        if (get_occupieds_at(q)) {
            if (s == 0) s = capacity() - 1;
            else --s;
            clear_runends_at(s); // this is here since we have to update occupieds at position q anyway so we always incur in a cache miss
        }
    } else { // slot is free (select1 not defined is a corner-case when the block is empty)
        fprintf(stderr, "no collisions, inserting r = %llu in q = %llu\n", rem, q);
        insert_in_empty_slot(q, rem);
        set_runends_at(q);
        // No update necessary because distance of cell from its runend is always 0 (they are at the same indexes)
    }
    set_occupieds_at(q);
    ++m_size;
    fprintf(stderr, "*******************************************************\n");
}

CLASS_HEADER
void
METHOD_HEADER::erase(key_type const& key)
{
    // TODO
    --m_size;
}

CLASS_HEADER
void 
METHOD_HEADER::clear()
{
    typename std::remove_reference<decltype(*this)>::type qf_new(m_hash_bitwidth, m_remainder_bitwidth);
    swap(qf_new);
}

CLASS_HEADER
void 
METHOD_HEADER::swap(METHOD_HEADER& other)
{
    std::swap(m_load_factor, other.m_load_factor);
    std::swap(m_size, other.m_size);
    std::swap(m_hash_bitwidth, other.m_hash_bitwidth);
    std::swap(m_remainder_bitwidth, other.m_remainder_bitwidth);
    std::swap(blocks_data, other.blocks_data);
}

CLASS_HEADER
std::tuple<typename METHOD_HEADER::key_type, typename METHOD_HEADER::key_type>
METHOD_HEADER::get_quotient_and_remainder(key_type hval) const
{
    const key_type remainder = hval & ((static_cast<key_type>(1) << m_remainder_bitwidth) - 1);
    const key_type quotient = hval >> m_remainder_bitwidth;
    if (quotient >= capacity()) throw std::length_error("[get_quotient_and_remainder] quotient out of bounds");
    return std::make_tuple(quotient, remainder);
}

CLASS_HEADER
void
METHOD_HEADER::set_occupieds_at(std::size_t idx)
{
    auto [block, local_idx] = index_to_block_coordinates(idx);
    *block.occupieds |= static_cast<UnderlyingType>(1) << local_idx;
}

CLASS_HEADER
void
METHOD_HEADER::clear_occupieds_at(std::size_t idx)
{
    auto [block, local_idx] = index_to_block_coordinates(idx);
    *block.occupieds &= ~(static_cast<UnderlyingType>(1) << local_idx);
}

CLASS_HEADER
bool
METHOD_HEADER::get_occupieds_at(std::size_t idx) const noexcept
{
    auto [block, local_idx] = index_to_block_coordinates(idx);
    return (*block.occupieds & (static_cast<UnderlyingType>(1) << local_idx)) != 0;
}

CLASS_HEADER
void
METHOD_HEADER::set_runends_at(std::size_t idx)
{
    auto [block, local_idx] = index_to_block_coordinates(idx);
    *block.runends |= static_cast<UnderlyingType>(1) << local_idx;
}

CLASS_HEADER
void
METHOD_HEADER::clear_runends_at(std::size_t idx)
{
    auto [block, local_idx] = index_to_block_coordinates(idx);
    *block.runends &= ~(static_cast<UnderlyingType>(1) << local_idx);
}

CLASS_HEADER
bool
METHOD_HEADER::get_runends_at(std::size_t idx) const noexcept
{
    auto [block, local_idx] = index_to_block_coordinates(idx);
    return (*block.runends & (static_cast<UnderlyingType>(1) << local_idx)) != 0;
}

CLASS_HEADER
std::size_t
METHOD_HEADER::get_remainder_at(std::size_t idx) const
{
    auto [block, local_idx] = index_to_block_coordinates(idx);
    return get_remainder_at(block, local_idx);
}

CLASS_HEADER
void
METHOD_HEADER::insert_in_empty_slot(std::size_t idx, key_type remainder)
{
    auto [block, local_idx] = index_to_block_coordinates(idx);
    set_remainder_at(block, local_idx, remainder);
}

CLASS_HEADER
std::pair<typename METHOD_HEADER::key_type, bool> 
METHOD_HEADER::right_shift_slots_between(std::size_t start, std::size_t stop, std::pair<key_type, bool> carry)
{
    //start and stop are INCLUDED in the range (stop is the free slot)
    // this method suppose there is enough space for shifting one position to the right
    // fprintf(stderr, "shifting from %llu to %llu and inserting %llu\n", start, stop, remainder);
    auto [start_block_idx, start_local_idx] = index_to_block_indexes(start);
    auto [stop_block_idx, stop_local_idx] = index_to_block_indexes(stop);

    auto block = block_at(start_block_idx);
    key_type remainder = carry.first;
    bool carry_runend = carry.second;

    if (start_block_idx != stop_block_idx) {
        bool ncarry = (*block.runends & last_bit_mask) != 0; // save carry bit for the next block
        { // shift in the first block
            auto unaffected_mask = (static_cast<UnderlyingType>(1) << start_local_idx) - 1; // mask for the unaffected bits by the shift
            *block.runends = 
                (*block.runends & unaffected_mask) | 
                ((*block.runends & ~unaffected_mask) << 1
            );
            remainder = right_shift_remainders(block, start_local_idx, remainders_per_block - 1, remainder); // this also sets the remainder at start_local_idx of block (so, at start position)
            if (carry_runend) set_runends_at(start); // do it here and not outside for better cache locality
            if (*block.offset >= start_local_idx) ++(*block.offset);
            carry_runend = ncarry;
        }
        for (std::size_t i = start_block_idx + 1; i != stop_block_idx; ++i) { // shift whole blocks between start -- end
            block = block_at(i);
            ncarry = (*block.runends & last_bit_mask) != 0;
            *block.runends <<= 1;
            if (carry_runend) *block.runends |= static_cast<UnderlyingType>(1);
            carry_runend = ncarry;
            remainder = right_shift_remainders(block, 0, remainders_per_block - 1, remainder);
            ++(*block.offset);
        }

        block = block_at(stop_block_idx);
        {
            auto affected_mask = (static_cast<UnderlyingType>(1) << stop_local_idx) - 1;
            auto buffer_runends = (*block.runends & affected_mask) << 1;
            ncarry = (*block.runends & (static_cast<UnderlyingType>(1) << stop_local_idx)) != 0;
            if (carry_runend) buffer_runends |= static_cast<UnderlyingType>(1);
            carry_runend = ncarry;
            // we know that occupieds and runends are 0 at stop_block_idx by construction (and enforced by an exception in insert)
            // so the msb of buffer will always end up on a clear bit --> no need to mask buffer variables to make sure
            *block.runends = (*block.runends & ~affected_mask) | buffer_runends;
            remainder = right_shift_remainders(block, 0, stop_local_idx, remainder);
            ++(*block.offset);
        }
    } else {
        bool ncarry = (*block.runends & (static_cast<UnderlyingType>(1) << stop_local_idx)) != 0;
        auto unaffected_mask = 
            ((static_cast<UnderlyingType>(1) << start_local_idx) - 1) | // lower part
            ~((static_cast<UnderlyingType>(1) << stop_local_idx) - 1);  // upper part
        *block.runends = (*block.runends & unaffected_mask) | ((*block.runends & ~unaffected_mask) << 1);
        if (carry_runend) set_runends_at(start);
        carry_runend = ncarry;
        remainder = right_shift_remainders(block, start_local_idx, stop_local_idx, remainder);
        if (start_local_idx <= *block.offset and *block.offset <= stop_local_idx) ++(*block.offset);
    }
    return {remainder, carry_runend};
}

CLASS_HEADER
std::size_t
METHOD_HEADER::find_runend(std::size_t idx) const
{
    const auto cap = capacity();
    idx = idx % cap;
    auto [block_idx, local_idx] = index_to_block_indexes(idx);
    auto block = block_at(block_idx);
    // fprintf(stderr, "block.occupieds = %llu\n", *block.occupieds);
    auto local_rank = bit::rank1(static_cast<uint64_t>(*block.occupieds), local_idx);
    auto select_idx_start = (idx - local_idx + *block.offset); // go to the first index in the block and add its (sampled) offset
    fprintf(stderr, "idx = %llu, block_idx = %llu, local_idx (inside block) = %llu, block_offset = %u, local_rank = %llu, select_idx_start = %llu\n", 
        idx, 
        block_idx,
        local_idx,
        *block.offset, 
        local_rank, 
        select_idx_start
    );

    // find the local_rank-th set bit in runends from position select_idx_start
    std::tie(block_idx, local_idx) = index_to_block_indexes(select_idx_start % cap);
    auto bitr = block_at(block_idx);
    auto select_query = *bitr.runends >> local_idx;
    auto pcnt = bit::popcount(select_query);
    fprintf(stderr, "block from which to start to select = %llu, local_idx = %llu, runends = %llu, popcount(%llu) = %u\n", 
        block_idx,
        local_idx,
        *bitr.runends,
        select_query, 
        pcnt
    );
    while (pcnt < local_rank) { // shift blocks and add their rank until we find block we want
        local_rank -= pcnt;
        select_idx_start += remainders_per_block - local_idx;
        std::tie(block_idx, local_idx) = index_to_block_indexes(select_idx_start % cap);
        bitr = block_at(block_idx);
        select_query = *bitr.runends >> local_idx;
        pcnt = bit::popcount(select_query);
    }
    fprintf(stderr, "idx = %llu, final block_idx = %llu, local_idx (inside block) = %llu, block_offset = %u, final local_rank = %llu, select_idx_start = %llu, select_query = %llu\n", 
        idx, 
        block_idx,
        local_idx,
        *block.offset, 
        local_rank, 
        select_idx_start,
        select_query
    );
    std::size_t res = idx;
    if (local_rank) {
        res = select_idx_start + bit::select1(select_query, local_rank - 1);
        if (res < idx) res = idx;
        else res = (res + 1) % cap;
    }
    fprintf(stderr, "runend(%llu) = %llu\n", idx, res);
    return res;
}

CLASS_HEADER
std::size_t
METHOD_HEADER::blocks_size() const noexcept
{
    std::size_t x = capacity() / remainders_per_block;
    return (x != 0) ? x : 1;
}

CLASS_HEADER
std::size_t 
METHOD_HEADER::block_byte_size() const noexcept
{
    block_t dummy;
    return 
        (remainders_per_block * m_remainder_bitwidth) / 8 + 
        sizeof(*dummy.offset) + 
        sizeof(*dummy.occupieds) + 
        sizeof(*dummy.runends);
}

CLASS_HEADER
typename METHOD_HEADER::block_t
METHOD_HEADER::block_at(std::size_t idx) const
{
    auto bidx = block_byte_size() * idx;
    if (bidx >= blocks_data.size()) throw std::out_of_range("[block_at] trying to access out-of-bounds block");
    block_t block;
    uint8_t* block_start_ptr = const_cast<uint8_t*>(blocks_data.data()) + block_byte_size() * idx; // for simplicity
    block.offset = reinterpret_cast<uint8_t*>(block_start_ptr);
    block.occupieds = reinterpret_cast<UnderlyingType*>(block_start_ptr + sizeof(uint8_t));
    block.runends = reinterpret_cast<UnderlyingType*>(block_start_ptr + sizeof(uint8_t) + sizeof(UnderlyingType));
    block.remainders = reinterpret_cast<remainder_pack_t*>(block_start_ptr + sizeof(uint8_t) + 2*sizeof(UnderlyingType));
    return block;
}

CLASS_HEADER
std::tuple<std::size_t, std::size_t>
METHOD_HEADER::index_to_block_indexes(std::size_t idx) const noexcept
{
    return std::make_tuple(idx / remainders_per_block, idx % remainders_per_block);
}

CLASS_HEADER
std::tuple<typename METHOD_HEADER::block_t, std::size_t> 
METHOD_HEADER::index_to_block_coordinates(std::size_t idx) const noexcept
{
    auto [block_idx, local_idx] = index_to_block_indexes(idx);
    return std::make_tuple(block_at(block_idx), local_idx);
}

CLASS_HEADER
std::tuple<std::size_t, long long> 
METHOD_HEADER::index_to_remainder_coordinates(std::size_t remainder_index) const noexcept
{
    std::size_t ut_idx = (remainder_index * m_remainder_bitwidth) / remainders_per_block;
    long long ut_shift = remainders_per_block - m_remainder_bitwidth - ((remainder_index * m_remainder_bitwidth) % remainders_per_block);
    return std::make_tuple(ut_idx, ut_shift);
}

CLASS_HEADER
std::size_t
METHOD_HEADER::get_remainder_at(block_t const& block, std::size_t position) const
{
    if (position >= remainders_per_block) throw std::out_of_range("[get_remainder_at] Trying to access out of bounds remainder in block");
    auto [idx, shift] = index_to_remainder_coordinates(position);
    // fprintf(stderr, "position = %llu, UT idx = %llu, shift = %lld\n", position, idx, shift);
    // fprintf(stderr, "remainders[%llu] = %llu\n", idx, block.remainders[idx]);
    UnderlyingType buffer = block.remainders[idx];
    if (shift < 0) { // crossing border
        remainder_pack_t mask_shift = m_remainder_bitwidth + shift;
        buffer &= (remainder_pack_t(1) << mask_shift) - 1;
        buffer <<= -shift;
        buffer |= block.remainders[idx + 1] >> (remainders_per_block + shift);
    } else { // perfect fit
        buffer >>= shift;
        if (m_remainder_bitwidth + shift != remainders_per_block) { // mask if it does not exactly fits
            buffer &= ((static_cast<remainder_pack_t>(1) << m_remainder_bitwidth) - 1);
        }
    }
    return static_cast<std::size_t>(buffer);
}

CLASS_HEADER
void
METHOD_HEADER::set_remainder_at(block_t& block, std::size_t position, std::size_t val)
{
    assert(val == 0 or bit::msbll(val) < m_remainder_bitwidth);
    auto [idx, shift] = index_to_remainder_coordinates(position);
    // fprintf(stderr, "position = %llu, UT idx = %llu, shift = %lld\n", position, idx, shift);
    // fprintf(stderr, "inserting : %llu\n", val);
    // fprintf(stderr, "remainders[%llu] = %llu\n", idx, block.remainders[idx]);
    if (shift < 0) { // crossing border
        remainder_pack_t mask_shift = m_remainder_bitwidth + shift;
        block.remainders[idx] &= ~((remainder_pack_t(1) << mask_shift) - 1);
        block.remainders[idx] |= static_cast<remainder_pack_t>(val >> -shift);
        block.remainders[idx + 1] &= ((remainder_pack_t(1) << (remainders_per_block + shift)) - 1);
        block.remainders[idx + 1] |= val << (remainders_per_block + shift);//(remainders_per_block - m_remainder_bitwidth);
    } else { // perfect fit
        remainder_pack_t buffer = val << shift;
        buffer |= block.remainders[idx] & ((static_cast<remainder_pack_t>(1) << shift) - 1); 
        if (m_remainder_bitwidth + shift != remainders_per_block) 
            buffer |= block.remainders[idx] & ~((static_cast<remainder_pack_t>(1) << (m_remainder_bitwidth + shift)) - 1);
        block.remainders[idx] = buffer;
    }
    // fprintf(stderr, "remainders[idx] = %llu\n", block.remainders[idx]);
}

CLASS_HEADER
std::size_t 
METHOD_HEADER::right_shift_remainders(block_t& block, std::size_t start, std::size_t stop, std::size_t val)
{
    auto carry = get_remainder_at(block, stop);
    while(stop != start) { // FIXME do it by UnderlyingType for better performances
        set_remainder_at(block, stop, get_remainder_at(block, stop - 1));
        --stop;
    }
    set_remainder_at(block, start, val);
    return carry;
}

CLASS_HEADER
METHOD_HEADER::const_iterator::const_iterator(rank_select_quotient_filter const* parent, bool start)
    : 
    parent_filter(parent), 
    idx(0), 
    runend_idx(0)
{
    if (start and parent_filter->size()) find_next_run();
    else idx = parent_filter->capacity();
}

CLASS_HEADER
typename METHOD_HEADER::key_type
METHOD_HEADER::const_iterator::operator*() const
{
    auto remainder = parent_filter->get_remainder_at(runend_idx);
    // fprintf(stderr, "%llu|%llu\n", idx, remainder);
    return (idx << parent_filter->m_remainder_bitwidth) | remainder;
}

CLASS_HEADER
typename METHOD_HEADER::const_iterator const&
METHOD_HEADER::const_iterator::operator++() noexcept
{
    // fprintf(stderr, "[before increment] idx = %llu, runend = %llu\n", idx, runend_idx);
    if (idx < parent_filter->capacity()) {
        --runend_idx;
        auto expected_end = (idx != 0) ? idx - 1 : parent_filter->capacity() - 1;
        if (runend_idx == std::numeric_limits<decltype(runend_idx)>::max()) runend_idx = parent_filter->capacity() - 1;
        if (parent_filter->get_runends_at(runend_idx)) { // the whole run is shifted and it does not end in idx
            // idx = runend_idx + 1;
            ++idx;
            if (idx < parent_filter->capacity()) find_next_run();
        } else if (runend_idx == expected_end) { // the run started at idx
            ++idx; // start looking from the index after idx
            if (idx < parent_filter->capacity()) find_next_run();
        }
    }
    // fprintf(stderr, "[after increment] idx = %llu, runend = %llu\n", idx, runend_idx);
    return *this;
}

CLASS_HEADER
typename METHOD_HEADER::const_iterator
METHOD_HEADER::const_iterator::operator++(int) noexcept
{
    auto current = *this;
    operator++();
    return current;
}

CLASS_HEADER
void
METHOD_HEADER::const_iterator::find_next_run()
{
    auto [block_idx, local_shift] = parent_filter->index_to_block_indexes(idx);
    auto occ_buffer = *parent_filter->block_at(block_idx).occupieds >> local_shift;
    // fprintf(stderr, "occupieds = %llu, shift = %llu\n", *parent_filter->block_at(block_idx).occupieds, local_shift);
    while (idx < parent_filter->capacity() and not (occ_buffer & static_cast<UnderlyingType>(1))) {
        ++idx;
        if (idx < parent_filter->capacity()) {
            if (idx % parent_filter->remainders_per_block == 0) {
                std::tie(block_idx, local_shift) =  parent_filter->index_to_block_indexes(idx);
                assert(local_shift == 0);
                occ_buffer = *parent_filter->block_at(block_idx).occupieds;
            } else {
                occ_buffer >>= 1;
            }
        }
    }
    runend_idx = parent_filter->find_runend(idx) - 1;
    if (runend_idx == std::numeric_limits<decltype(runend_idx)>::max()) runend_idx = parent_filter->capacity() - 1;
}

CLASS_HEADER
void
METHOD_HEADER::print_metadata() const noexcept
{
    for (std::size_t i = 0; i < blocks_size(); ++i) {
        auto block = block_at(i);
        fprintf(stderr, "%lu ", *block.offset);
    }
    fprintf(stderr, "\n");
    for (std::size_t i = 0; i < blocks_size(); ++i) {
        auto block = block_at(i);
        toolbox::print_bits_reverse(*block.occupieds);
    }
    fprintf(stderr, "\n");
    for (std::size_t i = 0; i < blocks_size(); ++i) {
        auto block = block_at(i);
        toolbox::print_bits_reverse(*block.runends);
    }
    fprintf(stderr, "\n");
}

CLASS_HEADER
void
METHOD_HEADER::print_remainders() const noexcept
{
    for (std::size_t i = 0; i < capacity(); ++i) {
        fprintf(stderr, "remainders[%llu] = %llu\n", i, get_remainder_at(i));
    }
    fprintf(stderr, "\n");
}

#undef METHOD_HEADER
#undef CLASS_HEADER

} // namespace approximate
} // namespace membership

#endif // RANK_SELECT_QUOTIENT_FILTER_HPP