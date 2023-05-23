#ifndef RANK_SELECT_HPP
#define RANK_SELECT_HPP

#include <cmath>
#include <vector>
#include "bit_operations.hpp"
#include "packed_vector.hpp"

#include <iostream>

namespace bit {

template <typename BitVector, std::size_t block_bit_size, std::size_t super_block_block_size>
class rank_select
{
    public:
        using bv_type = BitVector;
        // using block_bit_size = ; // view raw bit vectors as vectors of block_view_type integers
        // using super_block_block_size = uint16_t; // type for storing 1 counts in super-blocks

        rank_select(BitVector&& vector);
        std::size_t rank1(std::size_t idx) const;
        std::size_t rank0(std::size_t idx) const;
        std::size_t select1(std::size_t idx) const;
        // std::size_t select0(std::size_t idx) const;
        std::size_t size() const noexcept;
        std::size_t size0() const noexcept;
        std::size_t size1() const noexcept;

        BitVector const& data() const noexcept;

        template <class Visitor>
        void visit(Visitor& visitor) const;

    private:
        const BitVector _data;
        packed::vector<block_bit_size> blocks;
        packed::vector<block_bit_size> super_blocks;

        void build_index();
};

template <typename BitVector, std::size_t block_bit_size, std::size_t super_block_block_size>
rank_select<BitVector, block_bit_size, super_block_block_size>::rank_select(BitVector&& vector) 
    : _data(vector)
{
    build_index();
}

template <typename BitVector, std::size_t block_bit_size, std::size_t super_block_block_size>
std::size_t 
rank_select<BitVector, block_bit_size, super_block_block_size>::rank1(std::size_t idx) const
{
    assert(idx < _data.size());
    std::size_t super_block_idx = idx / (super_block_block_size * block_bit_size);
    std::size_t block_idx = idx / block_bit_size;
    std::size_t super_rank = super_blocks.template at<std::size_t>(super_block_idx);
    std::size_t block_rank = blocks.template at<std::size_t>(block_idx);
    std::cerr << "super rank [" << super_block_idx << "] = " << super_rank << "\n";
    std::cerr << "block rank [" << block_idx << "] = " << block_rank << "\n";
    std::size_t local_rank = 0;
    // std::size_t index = idx / block_bit_size * block_bit_size;
    for (auto itr = _data.cbegin() + idx / block_bit_size * block_bit_size; itr != _data.cbegin() + idx; ++itr) {
        // assert(_data.at(index) == *itr);
        local_rank += *itr; // no pre-computed table here
    }
    std::cerr << "local rank =" << local_rank << "\n";
    return super_rank + block_rank + local_rank;
}

template <typename BitVector, std::size_t block_bit_size, std::size_t super_block_block_size>
std::size_t 
rank_select<BitVector, block_bit_size, super_block_block_size>::rank0(std::size_t idx) const
{
    return idx - rank1(idx);
}

template <typename BitVector, std::size_t block_bit_size, std::size_t super_block_block_size>
std::size_t 
rank_select<BitVector, block_bit_size, super_block_block_size>::select1(std::size_t th) const
{
    assert(th < size1());
    std::size_t a = 0;
    std::size_t b = _data.size();
    while(b - a > 1) {
        std::size_t mid = a + (b - a) / 2;
        std::size_t x = rank1(mid);
        if (x <= th) {
            a = mid;
        } else {
            b = mid;
        }
    }
    return a;
}

// template <typename BitVector, std::size_t block_bit_size, std::size_t super_block_block_size>
// std::size_t 
// rank_select<BitVector, block_bit_size, super_block_block_size>::select0(std::size_t idx) const
// {
//     //
// }

template <typename BitVector, std::size_t block_bit_size, std::size_t super_block_block_size>
std::size_t 
rank_select<BitVector, block_bit_size, super_block_block_size>::size() const noexcept
{
    return _data.size();
}

template <typename BitVector, std::size_t block_bit_size, std::size_t super_block_block_size>
std::size_t 
rank_select<BitVector, block_bit_size, super_block_block_size>::size0() const noexcept
{
    return rank0(_data.size());
}

template <typename BitVector, std::size_t block_bit_size, std::size_t super_block_block_size>
std::size_t 
rank_select<BitVector, block_bit_size, super_block_block_size>::size1() const noexcept
{
    return rank1(_data.size());
}

template <typename BitVector, std::size_t block_bit_size, std::size_t super_block_block_size>
BitVector const&
rank_select<BitVector, block_bit_size, super_block_block_size>::data() const noexcept
{
    return _data;
}

template <typename BitVector, std::size_t block_bit_size, std::size_t super_block_block_size>
void 
rank_select<BitVector, block_bit_size, super_block_block_size>::build_index()
{
    const std::size_t super_block_bit_size = super_block_block_size * block_bit_size;
    std::size_t prev_block_count = 0;
    std::size_t prev_super_block_count = 0;
    std::size_t cumulative_block_count = 0;
    std::size_t index = 0;
    for (auto itr = _data.cbegin(); itr != _data.cend(); ++itr, ++index) {
        if (index != 0 and index % block_bit_size == 0) {
            blocks.push_back(prev_block_count); // push previous block count
            prev_block_count = cumulative_block_count;
            if (index % super_block_bit_size == 0) { // closing super-block
                super_blocks.push_back(prev_super_block_count);
                // std::cerr << "pushing " << prev_super_block_count << "\n";
                prev_super_block_count += cumulative_block_count; // this is the count of the super-block before reinitialisation
                prev_block_count = 0;
                cumulative_block_count = 0;
            }
        }
        if (*itr) ++cumulative_block_count;
    }
    // Padding with up-to (super_block_block_size * block_bit_size)
    if (_data.size() % super_block_bit_size) {
        // auto round_up = (_data.size() % super_block_bit_size + 1) * super_block_bit_size;
        auto padding_bits = super_block_bit_size - _data.size() % super_block_bit_size;
        // std::cerr << "round up = " << round_up << ", padding = " << padding_bits << "\n";
        for (; index < _data.size() + padding_bits; ++index) {
            if (index != 0 and index % block_bit_size == 0) {
                blocks.push_back(prev_block_count); // push previous block count
                prev_block_count = cumulative_block_count;
                if (index % super_block_bit_size == 0) { // closing super-block
                    super_blocks.push_back(prev_super_block_count);
                    prev_super_block_count = cumulative_block_count; // this is the count of the super-block before reinitialisation
                    prev_block_count = 0;
                    cumulative_block_count = 0;
                }
            }
            // all padding bits are 0 so no increment of cumulative_block_count
        }
    }
    super_blocks.push_back(prev_super_block_count);
    // std::cerr << "prev super block size = " << prev_super_block_count << "\n";
    // std::cerr << "data size = " << _data.size() << ", index = " << index << ", sb bit size = " << super_block_bit_size << "\n";
    assert(index % super_block_bit_size == 0);
    // std::cerr << "blocks size = " << blocks.size() << ", super blocks size = " << super_blocks.size() << "\n";
}

template <typename BitVector, std::size_t block_bit_size, std::size_t super_block_block_size>
template <class Visitor>
void 
rank_select<BitVector, block_bit_size, super_block_block_size>::visit(Visitor& visitor) const
{
    visitor.apply(_data);
    visitor.apply(blocks);
    visitor.apply(super_blocks);
}

} // namespace bit

#endif // RANK_SELECT_HPP