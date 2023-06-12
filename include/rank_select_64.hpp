#pragma once // just to not pollute the global scope with flags

#include <cassert>
#include "select_hints.hpp"
#include "logtools.hpp"

namespace bit {
namespace rs {

// Specialisation working with this library's bit-vectors made of 64-bits blocks.
template <bool with_select_hints>
class array<bit::vector<uint64_t>, 64, 8, with_select_hints> : protected select_hints<with_select_hints>
{
    public:
        using bv_type = bit::vector<uint64_t>;

        array(bit::vector<uint64_t>&& vector);
        array(array const&) = default;
        array(array&&) = default;
        std::size_t rank1(std::size_t idx) const;
        std::size_t rank0(std::size_t idx) const {return idx - rank1();}
        std::size_t select1(std::size_t th) const;
        std::size_t select0(std::size_t th) const;
        std::size_t size() const noexcept {return _data.size();}
        std::size_t size0() const noexcept {return size() - size1();}
        std::size_t size1() const noexcept {return *(interleaved_blocks.end() - 2);}
        std::size_t bit_size() const noexcept {return _data.size() + bit_overhead();}
        std::size_t bit_overhead() const noexcept; // {return interleaved_blocks.size() * sizeof(uint64_t) * 8;}

        bit::vector<uint64_t> const& data() const noexcept {return _data;}
        void swap(array& other);

        template <class Visitor>
        void visit(Visitor& visitor);

        template <class Visitor>
        void visit(Visitor& visitor) const;

    private:
        static const std::size_t block_bit_size = 64;
        static const std::size_t super_block_block_size = 8;
        static const uint64_t select_ones_per_hint = 64 * super_block_block_size * 2;  // must be > block_size * 64
        static const uint64_t select_zeros_per_hint = select_ones_per_hint;
        static const uint64_t ones_step_9 = 1ULL << 0 | 1ULL << 9 | 1ULL << 18 | 1ULL << 27 | 1ULL << 36 | 1ULL << 45 | 1ULL << 54;
        static const uint64_t msbs_step_9 = 0x100ULL * ones_step_9;
        bit::vector<uint64_t> const _data;
        std::vector<uint64_t> interleaved_blocks;

        void build_index();
        inline std::vector<uint64_t> const& payload() const noexcept {return _data.vector_data();}
        inline std::size_t super_blocks_size() const {return interleaved_blocks.size() / 2 - 1;}
        inline std::size_t super_block_rank(uint64_t super_block_idx) const {return interleaved_blocks.at(super_block_idx * 2);}
        inline std::size_t block_ranks(uint64_t super_block_idx) const {return interleaved_blocks.at(super_block_idx * 2 + 1);}
        inline std::size_t super_and_block_partial_rank(uint64_t block_idx) const {
            uint64_t r = 0;
            uint64_t block = block_idx / super_block_block_size;
            r += super_block_rank(block);
            uint64_t left = block_idx % super_block_block_size;
            r += block_ranks(block) >> ((7 - left) * 9) & 0x1FF;
            return r;
        }

        inline static uint64_t uleq_step_9(uint64_t x, uint64_t y) {
            return (((((y | msbs_step_9) - (x & ~msbs_step_9)) | (x ^ y)) ^ (x & ~y)) & msbs_step_9) >> 8;
        }
};

template <bool with_select_hints>
array<bit::vector<uint64_t>, 64, 8, with_select_hints>::array(bit::vector<uint64_t>&& vector)
    : _data(vector)
{
    build_index();
}

template <bool with_select_hints>
void 
array<bit::vector<uint64_t>, 64, 8, with_select_hints>::build_index()
{
    std::vector<uint64_t> block_rank_pairs;
    uint64_t next_rank = 0;
    uint64_t cur_subrank = 0;
    uint64_t subranks = 0;
    block_rank_pairs.push_back(0);
    for (uint64_t i = 0; i < payload().size(); ++i) {
        uint64_t word_pop = popcount(payload().at(i));
        uint64_t shift = i % super_block_block_size;
        if (shift) {
            subranks <<= 9;
            subranks |= cur_subrank;
        }
        next_rank += word_pop;
        cur_subrank += word_pop;

        if (shift == super_block_block_size - 1) {
            block_rank_pairs.push_back(subranks);
            block_rank_pairs.push_back(next_rank);
            subranks = 0;
            cur_subrank = 0;
        }
    }
    uint64_t left = super_block_block_size - payload().size() % super_block_block_size;
    for (uint64_t i = 0; i < left; ++i) {
        subranks <<= 9;
        subranks |= cur_subrank;
    }
    block_rank_pairs.push_back(subranks);

    if (payload().size() % super_block_block_size) {
        block_rank_pairs.push_back(next_rank);
        block_rank_pairs.push_back(0);
    }

    interleaved_blocks.swap(block_rank_pairs);

    if constexpr (with_select_hints) {
        std::vector<std::size_t> temp_hints;
        uint64_t cur_ones_threshold = select_ones_per_hint;
        for (std::size_t i = 0; i < super_blocks_size(); ++i) {
            if (super_block_rank(i + 1) > cur_ones_threshold) {
                temp_hints.push_back(i);
                cur_ones_threshold += select_ones_per_hint;
            }
        }
        temp_hints.push_back(super_blocks_size());
        select_hints<with_select_hints>::hints.swap(temp_hints);
    }
}

template <bool with_select_hints>
std::size_t 
array<bit::vector<uint64_t>, 64, 8, with_select_hints>::rank1(std::size_t idx) const
{
    assert(idx <= size());
    if (idx == size()) return size1();

    std::size_t sub_block = idx / block_bit_size;
    std::size_t r = super_and_block_partial_rank(sub_block);
    std::size_t sub_left = idx % block_bit_size;
    if (sub_left) r += popcount(payload().at(sub_block) << (block_bit_size - sub_left));
    return r;
}

template <bool with_select_hints>
std::size_t 
array<bit::vector<uint64_t>, 64, 8, with_select_hints>::select1(std::size_t th) const
{
    assert(th < size1());
    std::size_t a = 0;
    std::size_t b = super_blocks_size();

    if constexpr (with_select_hints) {
        std::size_t chunk = th / select_ones_per_hint;
        if (chunk != 0) a = select_hints<with_select_hints>::hints.at(chunk - 1);
        b = select_hints<with_select_hints>::hints.at(chunk) + 1;
    }

    while (b - a > 1) {
        std::size_t mid = a + (b - a) / 2;
        std::size_t x = super_block_rank(mid);
        if (x <= th) a = mid;
        else b = mid;
    }
    auto super_block_idx = a;
    assert(super_block_idx < super_blocks_size());

    std::size_t cur_rank = super_block_rank(super_block_idx);
    assert(cur_rank <= th);

    std::size_t rank_in_block_parallel = (th - cur_rank) * ones_step_9;
    auto block_rank = block_ranks(super_block_idx);
    uint64_t block_offset = uleq_step_9(block_rank, rank_in_block_parallel) * ones_step_9 >> 54 & 0x7;
    cur_rank += block_rank >> (7 - block_offset) * 9 & 0x1FF;
    assert(cur_rank <= th);

    uint64_t word_offset = super_block_idx * super_block_block_size + block_offset;
    uint64_t last = payload().at(word_offset);
    std::size_t remaining_bits = th - cur_rank;
    auto local_offset = select(last, remaining_bits);
    // std::cerr << 
    //     "payload[0] = " << payload().at(0) << 
    //     " == " << _data.vector_data().at(0) << 
    //     "\n" <<
    //     "word offset = " << word_offset << 
    //     ", last = " << last << 
    //     ", remaining bits = " << remaining_bits << 
    //     ", local offset = " << local_offset << 
    //     "\n";
    return word_offset * 64 + local_offset;
}

template <bool with_select_hints>
std::size_t 
array<bit::vector<uint64_t>, 64, 8, with_select_hints>::select0(std::size_t th) const
{
    assert(th < size0());
    std::size_t a = 0;
    std::size_t b = super_blocks_size();

    // 0s don't have hints !!!
    while (b - a > 1) {
        std::size_t mid = a + (b - a) / 2;
        std::size_t x = rank0(mid);
        if (x <= th) a = mid;
        else b = mid;
    }
    assert(a < size());
    return a;
}

template <bool with_select_hints>
std::size_t 
array<bit::vector<uint64_t>, 64, 8, with_select_hints>::bit_overhead() const noexcept 
{
    logging_tools::libra logger;
    visit(logger);
    return 8 * logger.get_byte_size() - _data.bit_size();
}

template <bool with_select_hints>
void 
array<bit::vector<uint64_t>, 64, 8, with_select_hints>::swap(array& other)
{
    _data.swap(other._data);
    interleaved_blocks.swap(other.interleaved_blocks);
}

template <bool with_select_hints>
template <class Visitor>
void 
array<bit::vector<uint64_t>, 64, 8, with_select_hints>::visit(Visitor& visitor)
{
    visitor.apply(_data);
    visitor.apply(interleaved_blocks);
    if constexpr (with_select_hints) visitor.apply(select_hints<with_select_hints>::hints);
}

template <bool with_select_hints>
template <class Visitor>
void 
array<bit::vector<uint64_t>, 64, 8, with_select_hints>::visit(Visitor& visitor) const
{
    visitor.apply(_data);
    visitor.apply(interleaved_blocks);
    if constexpr (with_select_hints) visitor.apply(select_hints<with_select_hints>::hints);
}

} // namespace rs
} // namespace bit