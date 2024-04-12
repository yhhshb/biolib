#ifndef RANK_SELECT_HPP
#define RANK_SELECT_HPP

#include <cmath>
#include <vector>
#include <cassert>
#include "constants.hpp"
#include "bit_operations.hpp"
#include "packed_vector.hpp"
#include "select_hints.hpp"
#include "logtools.hpp"

namespace bit {
namespace rs {

#define CLASS_HEADER template <typename BitVector, std::size_t block_bit_size, std::size_t super_block_block_size, bool with_select1_hints, bool with_select0_hints>
#define METHOD_HEADER array<BitVector, block_bit_size, super_block_block_size, with_select1_hints, with_select0_hints>

/**
 * Static bitvector rank/select data structure.
 * IMPORTANT rank(i) is defined as the number of 1 strictly before position i.
 */
CLASS_HEADER
class array 
    : protected select1_hints<with_select1_hints>, 
      protected select0_hints<with_select0_hints>
{
    public:
        using bv_type = BitVector;

        array(BitVector&& vector);
        array(array const&) noexcept= default;
        array(array&&) noexcept = default;
        array& operator=(array const&) noexcept = default;
        array& operator=(array&&) noexcept = default;
        std::size_t rank1(std::size_t idx) const;
        std::size_t rank0(std::size_t idx) const;
        std::size_t select1(std::size_t idx) const;
        std::size_t select0(std::size_t idx) const;
        std::size_t size() const noexcept;
        std::size_t size0() const noexcept;
        std::size_t size1() const noexcept;
        std::size_t bit_size() const noexcept;
        std::size_t bit_overhead() const noexcept;

        void swap(array& other) noexcept;

        BitVector const& data() const noexcept;
        // void swap(array& other);

        template <class Visitor>
        void visit(Visitor& visitor) const;

        template <class Visitor>
        void visit(Visitor& visitor);

        template <class Loader>
        static array load(Loader& visitor);

    private:
        BitVector _data;
        packed::vector<max_width_native_type> blocks;
        packed::vector<max_width_native_type> super_blocks;

        array();
        void build_index();

        friend bool operator==(array const& a, array const& b) 
        {
            bool same_data = a._data == b._data;
            bool same_blocks = a.blocks == b.blocks;
            bool same_super_blocks = a.super_blocks == b.super_blocks;
            bool result = same_data and same_blocks and same_super_blocks;
            if constexpr (with_select1_hints) {
                result &= a.hints1 == b.hints1;
            }
            if constexpr (with_select0_hints) {
                result &= a.hints0 == b.hints0;
            }
            return result;
        };
        friend bool operator!=(array const& a, array const& b) {return not (a == b);};

        // IMPROVEMENTS:
        // - pack each super-block and its blocks together in order to improve locality
        // - Write specialized class for above problem, replacing the packed vectors 
};

CLASS_HEADER
METHOD_HEADER::array()
    : blocks(packed::vector(static_cast<std::size_t>(std::ceil(std::log2(block_bit_size * super_block_block_size))))), 
      super_blocks(packed::vector(static_cast<std::size_t>(std::ceil(std::log2(_data.size())))))
{}

CLASS_HEADER
METHOD_HEADER::array(BitVector&& vector) 
    : _data(vector), 
      blocks(packed::vector(static_cast<std::size_t>(std::ceil(std::log2(block_bit_size * super_block_block_size))))), 
      super_blocks(packed::vector(static_cast<std::size_t>(std::ceil(std::log2(_data.size())))))
{
    build_index();
}

CLASS_HEADER
std::size_t 
METHOD_HEADER::rank1(std::size_t idx) const
{
    if (idx > _data.size()) throw std::out_of_range("[rank1] idx = " + std::to_string(idx) + " with size = " + std::to_string(_data.size()));
    std::size_t super_block_idx = idx / (super_block_block_size * block_bit_size);
    std::size_t block_idx = idx / block_bit_size;
    // std::size_t super_rank = super_blocks.template at<std::size_t>(super_block_idx);
    // std::size_t block_rank = blocks.template at<std::size_t>(block_idx);
    std::size_t super_rank = static_cast<std::size_t>(super_blocks.at(super_block_idx));
    std::size_t block_rank = static_cast<std::size_t>(blocks.at(block_idx));
    std::size_t local_rank = 0;
    for (auto itr = _data.cbegin() + block_idx * block_bit_size; itr != _data.cbegin() + idx; ++itr) local_rank += *itr; // no pre-computed table here
    // std::cerr << "super rank [" << super_block_idx << "] = " << super_rank << "\n";
    // std::cerr << "block rank [" << block_idx << "] = " << block_rank << "\n";
    // std::cerr << "local rank =" << local_rank << "\n";
    return super_rank + block_rank + local_rank;
}

CLASS_HEADER
std::size_t 
METHOD_HEADER::rank0(std::size_t idx) const
{
    return idx - rank1(idx);
}

CLASS_HEADER
std::size_t 
METHOD_HEADER::select1(std::size_t th) const
{
    assert(th < size1());
    std::size_t a = 0;
    std::size_t b = _data.size();
    while(b - a > 1) {
        std::size_t mid = a + (b - a) / 2;
        // std::cerr << "getting rank1 for idx = " << mid << "\n";
        std::size_t x = rank1(mid);
        if (x <= th) a = mid;
        else b = mid;
    }
    return a;
}

CLASS_HEADER
std::size_t 
METHOD_HEADER::select0(std::size_t th) const
{
    assert(th < size0());
    std::size_t a = 0;
    std::size_t b = _data.size();
    while(b - a > 1) {
        std::size_t mid = a + (b - a) / 2;
        std::size_t x = rank0(mid);
        if (x <= th) a = mid;
        else b = mid;
    }
    return a;
}

CLASS_HEADER
std::size_t 
METHOD_HEADER::size() const noexcept
{
    return _data.size();
}

CLASS_HEADER
std::size_t 
METHOD_HEADER::size0() const noexcept
{
    return size() - size1(); //rank0(_data.size() - 1) + (_data.at(_data.size() - 1) ? 0 : 1);
}

CLASS_HEADER
std::size_t 
METHOD_HEADER::size1() const noexcept
{
    if (_data.size() == 0) return 0;
    return rank1(_data.size() - 1) + static_cast<std::size_t>(_data.at(_data.size() - 1));
}

CLASS_HEADER
std::size_t 
METHOD_HEADER::bit_size() const noexcept
{
    logging_tools::libra logger;
    visit(logger);
    return 8 * logger.get_byte_size();
}

CLASS_HEADER
std::size_t 
METHOD_HEADER::bit_overhead() const noexcept
{
    return bit_size() - _data.bit_size();
}

CLASS_HEADER
void 
METHOD_HEADER::swap(array& other) noexcept
{
    _data.swap(other._data);
    blocks.swap(other.blocks);
    super_blocks.swap(other.super_blocks);
}

CLASS_HEADER
BitVector const&
METHOD_HEADER::data() const noexcept
{
    return _data;
}

// CLASS_HEADER
// void
// METHOD_HEADER::swap(array& other)
// {
//     _data.swap(other._data);
//     blocks.swap(other.blocks);
//     super_blocks.swap(other.super_blocks);
// }

CLASS_HEADER
void 
METHOD_HEADER::build_index()
{
    const std::size_t super_block_bit_size = super_block_block_size * block_bit_size;
    std::size_t prev_block_count = 0;
    std::size_t prev_super_block_count = 0;
    std::size_t cumulative_block_count = 0;
    std::size_t index = 0;
    for (auto itr = _data.cbegin(); itr != _data.cend(); ++itr, ++index) {
        if (index != 0 and index % block_bit_size == 0) {
            // std::cerr << "bit width = " << blocks.bit_width() << " ";
            // std::cerr << "rank of position " << index << " = " << prev_block_count << "\n";
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
    blocks.push_back(prev_block_count);
    super_blocks.push_back(prev_super_block_count);
    // std::cerr << "prev super block size = " << prev_super_block_count << "\n";
    // std::cerr << "data size = " << _data.size() << ", index = " << index << ", sb bit size = " << super_block_bit_size << "\n";
    assert(index % super_block_bit_size == 0);
    // std::cerr << "blocks size = " << blocks.size() << ", super blocks size = " << super_blocks.size() << "\n";
    // std::cerr << "blocks made of " << blocks.underlying_size() << " blocks of " << sizeof(bit::max_width_native_type) << " B\n";
    blocks.resize(blocks.size());
    super_blocks.resize(super_blocks.size());
    // std::cerr << "blocks bit size = " << blocks.bit_size() << ", super blocks bit size = " << super_blocks.bit_size() << "\n";

    // for (std::size_t i = 0; i < blocks.size(); ++i) { 
    //     std::cerr << blocks.template at<std::size_t>(i) << ", ";
    // }
    // std::cerr << "\n";
}

CLASS_HEADER
template <class Visitor>
void 
METHOD_HEADER::visit(Visitor& visitor)
{
    visitor.visit(_data);
    visitor.visit(blocks);
    visitor.visit(super_blocks);
    if constexpr (with_select1_hints) visitor.visit(select1_hints<with_select1_hints>::hints1);
    if constexpr (with_select0_hints) visitor.visit(select0_hints<with_select0_hints>::hints0);
}

CLASS_HEADER
template <class Visitor>
void 
METHOD_HEADER::visit(Visitor& visitor) const
{
    visitor.visit(_data);
    visitor.visit(blocks);
    visitor.visit(super_blocks);
    if constexpr (with_select1_hints) visitor.visit(select1_hints<with_select1_hints>::hints1);
    if constexpr (with_select0_hints) visitor.visit(select0_hints<with_select0_hints>::hints0);
}

CLASS_HEADER
template <class Loader>
METHOD_HEADER 
METHOD_HEADER::load(Loader& visitor)
{
    METHOD_HEADER r;
    r._data = decltype(r._data)::load(visitor);
    r.blocks = decltype(r.blocks)::load(visitor);
    r.super_blocks = decltype(r.super_blocks)::load(visitor);
    if constexpr (with_select1_hints) visitor.visit(r.hints1);
    if constexpr (with_select0_hints) visitor.visit(r.hints0);
    return r;
}

#undef CLASS_HEADER
#undef METHOD_HEADER

} // namespace rs
} // namespace bit

#include "rank_select_64.hpp"

#endif // RANK_SELECT_HPP