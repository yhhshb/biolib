#pragma once // just to not pollute the global scope with flags

#include "rank_select.hpp"

namespace bit {

template <>
class rank_select<bit::vector<uint64_t>, 64, 8>
{
    public:
        using bv_type = bit::vector<uint64_t>;

        rank_select(bit::vector<uint64_t>&& vector);
        std::size_t rank1(std::size_t idx) const;
        std::size_t rank0(std::size_t idx) const;
        std::size_t select1(std::size_t idx) const;
        // std::size_t select0(std::size_t idx) const;
        std::size_t size() const noexcept {return _data.size();}
        std::size_t size0() const noexcept {return rank0(_data.size());}
        std::size_t size1() const noexcept {return rank1(_data.size());}
        std::size_t bit_size() const noexcept {return _data.size() + bit_overhead();}
        std::size_t bit_overhead() const noexcept {return interleaved_blocks.size() * sizeof(uint64_t) * 8;}

        bit::vector<uint64_t> const& data() const noexcept {return _data;}

        template <class Visitor>
        void visit(Visitor& visitor) const;

    private:
        static const std::size_t block_bit_size = 64;
        static const std::size_t super_block_block_size = 8;
        bit::vector<uint64_t> const _data;
        std::vector<uint64_t> const& payload;
        std::vector<uint64_t> interleaved_blocks;

        void build_index();
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
};

rank_select<bit::vector<uint64_t>, 64, 8>::rank_select(bit::vector<uint64_t>&& vector)
    : _data(vector), payload(_data.vector_data())// Possible undefined behaviour
{
    build_index();
}

void 
rank_select<bit::vector<uint64_t>, 64, 8>::build_index()
{
    std::vector<uint64_t> block_rank_pairs;
    uint64_t next_rank = 0;
    uint64_t cur_subrank = 0;
    uint64_t subranks = 0;
    block_rank_pairs.push_back(0);
    for (uint64_t i = 0; i < payload.size(); ++i) {
        uint64_t word_pop = popcount(payload.at(i));
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
    uint64_t left = super_block_block_size - payload.size() % super_block_block_size;
    for (uint64_t i = 0; i < left; ++i) {
        subranks <<= 9;
        subranks |= cur_subrank;
    }
    block_rank_pairs.push_back(subranks);

    if (payload.size() % super_block_block_size) {
        block_rank_pairs.push_back(next_rank);
        block_rank_pairs.push_back(0);
    }

    interleaved_blocks.swap(block_rank_pairs);
}

std::size_t 
rank_select<bit::vector<uint64_t>, 64, 8>::rank1(std::size_t idx) const
{
    assert(idx <= size());
    if (idx == size()) return size1();

    uint64_t sub_block = idx / block_bit_size;
    uint64_t r = super_and_block_partial_rank(sub_block);
    uint64_t sub_left = idx % block_bit_size;
    if (sub_left) r += popcount(payload.at(sub_block) << (block_bit_size - sub_left));
    return r;
}

template <class Visitor>
void 
rank_select<bit::vector<uint64_t>, 64, 8>::visit(Visitor& visitor) const
{
    visitor.apply(_data);
    visitor.apply(interleaved_blocks);
}

} // namespace bit