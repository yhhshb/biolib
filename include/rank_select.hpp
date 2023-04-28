#ifndef RANK_SELECT_HPP
#define RANK_SELECT_HPP

#include <cmath>
#include <vector>
#include "bit_operations.hpp"

#include <iostream>

namespace bit {

template <typename BitVector, std::size_t block_byte_size, std::size_t super_block_block_size> // in multiples of 64 bit-words
class rank_select
{
    public:
        using block_view_type = uint64_t; // view raw bit vectors as vectors of block_view_type integers
        using rank_pack_sb_count_type = uint16_t; // type for storing 1 counts in super-blocks
        using rank_pack_bc_pack_type = uint64_t; // type for storing packed block counts for super-blocks (the longer the better)

        rank_select(BitVector&& vector);
        std::size_t rank1(std::size_t idx) const;
        std::size_t rank0(std::size_t idx) const;
        std::size_t select1(std::size_t idx) const;
        std::size_t select0(std::size_t idx) const;
        std::size_t size() const noexcept;
        std::size_t size0() const noexcept;
        std::size_t size1() const noexcept;

        template <class Visitor>
        void visit(Visitor& visitor);

    private:
        struct rank_pack_t {
            block_view_type sb; // super-block count
            uint64_t bc; // packed block counts of blocks belonging to the same super-block
        };

        const std::size_t super_block_byte_size;
        const BitVector _data;
        std::size_t pack_shift;
        std::vector<rank_pack_t> block_tree;

        void build_index();
};

template <typename BitVector, std::size_t block_byte_size, std::size_t super_block_block_size>
rank_select<BitVector, block_byte_size, super_block_block_size>::rank_select(BitVector&& vector) 
    : super_block_byte_size(super_block_block_size * block_byte_size), _data(vector)
{
    pack_shift = static_cast<std::size_t>(std::ceil(std::log2(8 * super_block_byte_size)));
    assert(super_block_byte_size % block_byte_size == 0);
    assert(super_block_byte_size >= bucket_byte_size);
    build_index();
}

template <typename BitVector, std::size_t block_byte_size, std::size_t super_block_block_size>
std::size_t 
rank_select<BitVector, block_byte_size, super_block_block_size>::rank1(std::size_t idx) const
{
    assert(idx < _data.size());
    // std::size_t super_block_idx = idx / (8 * super_block_byte_size);
    // std::size_t res = block_tree.at(super_block_idx).sb;
}

template <typename BitVector, std::size_t block_byte_size, std::size_t super_block_block_size>
std::size_t 
rank_select<BitVector, block_byte_size, super_block_block_size>::rank0(std::size_t idx) const
{
    return idx - rank1(idx);
}

template <typename BitVector, std::size_t block_byte_size, std::size_t super_block_block_size>
std::size_t 
rank_select<BitVector, block_byte_size, super_block_block_size>::select1(std::size_t idx) const
{
    //
}

template <typename BitVector, std::size_t block_byte_size, std::size_t super_block_block_size>
std::size_t 
rank_select<BitVector, block_byte_size, super_block_block_size>::select0(std::size_t idx) const
{
    //
}

template <typename BitVector, std::size_t block_byte_size, std::size_t super_block_block_size>
void 
rank_select<BitVector, block_byte_size, super_block_block_size>::build_index()
{
    std::size_t count_from_start_of_block = 0;
    std::size_t count_from_start_of_super_block = 0;
    std::size_t cumulative_block_count = 0;
    std::size_t cumulative_super_block_count = 0;
    std::size_t index;
    for (auto itr = _data.cbegin(), index = 0; itr != _data.cend(); ++itr, ++index) {
        if (*itr) ++cumulative_block_count;
        if (index != 0 and index % block_byte_size == 0) {
            cumulative_super_block_count += block_byte_size;
        }
    }
}

template <typename BitVector, std::size_t block_byte_size, std::size_t super_block_block_size>
template <class Visitor>
void 
rank_select<BitVector, block_byte_size, super_block_block_size>::visit(Visitor& visitor)
{
    visitor.apply(_data);
    visitor.apply(block_tree);
}

} // namespace bit

#endif // RANK_SELECT_HPP

/* keep it for specialized version equivalent to Giulio's (super block size = 512 bit and block size = 64)
template <typename BitVector, std::size_t block_byte_size, std::size_t super_block_block_size>
void 
rank_select<BitVector, block_byte_size, super_block_block_size>::build_index()
{
    uint8_t const * const data_view = reinterpret_cast<uint8_t const *>(_data.data());
    const data_byte_size = _data.size() / 8 + 1;
    rank_pack_t rp = {0, 0};
    std::size_t partial_subrank = 0;
    for(std::size_t i = 0; i < data_byte_size / block_byte_size; ++i) {
        auto block_index = i * block_byte_size;
        std::size_t word_rank = bit::popcount(data_view[block_index], block_byte_size);
        rp.sb += word_rank;
        partial_subrank += word_rank;
        if (i % super_block_byte_size) {
            rp.bc <<= pack_shift;
            rp.bc |= partial_subrank;
        } else {
            block_tree.push_back(rp);
            rp = {0, 0};
            partial_subrank = 0;
        }
    }
    std::size_t i = data_byte_size / block_byte_size * block_byte_size;
    std::size_t last_len = data_byte_size - i;
    if (last_len) {
        std::size_t word_rank = bit::popcount(data_view[i], last_len);
        rp.sb += word_rank;
        partial_subrank += word_rank;
        rp.bc <<= pack_shift;
        rp.bc |= partial_subrank;
        block_tree.push_back(rp);
    }
}
*/