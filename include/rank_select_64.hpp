#pragma once // just to not pollute the global scope with specializations flags

namespace bit {
namespace rs {

#define CLASS_HEADER template <bool with_select1_hints, bool with_select0_hints>
#define METHOD_HEADER array<bit::vector<uint64_t>, 64, 8, with_select1_hints, with_select0_hints>

// Specialisation working with this library's bit-vectors made of 64-bits blocks.
CLASS_HEADER
class array<bit::vector<uint64_t>, 64, 8, with_select1_hints, with_select0_hints> 
    : protected select1_hints<with_select1_hints>, 
      protected select0_hints<with_select0_hints>
{
    public:
        using bv_type = bit::vector<uint64_t>;

        array(bit::vector<uint64_t>&& vector) : _data(vector) {build_index();}
        array(array const&) noexcept = default;
        array(array&&) noexcept = default;
        array& operator=(array const&) noexcept = default;
        array& operator=(array&&) noexcept = default;
        std::size_t rank1(std::size_t idx) const;
        std::size_t rank0(std::size_t idx) const {return idx - rank1(idx);}
        std::size_t select1(std::size_t th) const;
        std::size_t select0(std::size_t th) const;
        std::size_t size() const noexcept {return _data.size();}
        std::size_t size0() const noexcept {return size() - size1();}
        std::size_t size1() const noexcept {return *(interleaved_blocks.end() - 2);}
        std::size_t bit_size() const noexcept;
        std::size_t bit_overhead() const noexcept {return bit_size() - _data.bit_size();}

        void swap(array& other) noexcept;

        bit::vector<uint64_t> const& data() const noexcept {return _data;}
        // void swap(array& other);

        template <class Visitor>
        void visit(Visitor& visitor) const;

        template <class Visitor>
        void visit(Visitor& visitor);

        template <class Loader>
        static array load(Loader& visitor);

    private:
        static const std::size_t block_bit_size = 64;
        static const std::size_t super_block_block_size = 8;
        static const uint64_t select_ones_per_hint = 64 * super_block_block_size * 2;  // must be > block_size * 64
        static const uint64_t select_zeros_per_hint = select_ones_per_hint;
        static const uint64_t ones_step_9 = 1ULL << 0 | 1ULL << 9 | 1ULL << 18 | 1ULL << 27 | 1ULL << 36 | 1ULL << 45 | 1ULL << 54;
        static const uint64_t msbs_step_9 = 0x100ULL * ones_step_9;
        bit::vector<uint64_t> _data;
        std::vector<uint64_t> interleaved_blocks;

        array() {}
        void build_index();
        inline std::vector<uint64_t> const& payload() const noexcept {return _data.vector_data();}
        inline std::size_t super_blocks_size() const {return interleaved_blocks.size() / 2 - 1;}
        inline uint64_t super_block_rank1(uint64_t super_block_idx) const {return interleaved_blocks.at(super_block_idx * 2);}
        inline std::size_t super_block_rank0(uint64_t super_block_idx) const {return block_bit_size * super_block_block_size * super_block_idx - super_block_rank1(super_block_idx);}
        inline uint64_t block_ranks(uint64_t super_block_idx) const {return interleaved_blocks.at(super_block_idx * 2 + 1);}
        inline uint64_t super_and_block_partial_rank(uint64_t block_idx) const {
            uint64_t r = 0;
            uint64_t block = block_idx / super_block_block_size;
            r += super_block_rank1(block);
            uint64_t left = block_idx % super_block_block_size;
            r += block_ranks(block) >> ((7 - left) * 9) & 0x1FF;
            return r;
        }

        inline static uint64_t uleq_step_9(uint64_t x, uint64_t y) {
            return (((((y | msbs_step_9) - (x & ~msbs_step_9)) | (x ^ y)) ^ (x & ~y)) & msbs_step_9) >> 8;
        }

        friend bool operator==(array const& a, array const& b) 
        {
            bool same_data = a._data == b._data;
            bool same_interleaved_blocks = a.interleaved_blocks == b.interleaved_blocks;
            bool result = same_data and same_interleaved_blocks;
            if constexpr (with_select1_hints) {
                result &= a.hints1 == b.hints1;
            }
            if constexpr (with_select0_hints) {
                result &= a.hints0 == b.hints0;
            }
            return result;
        };
        friend bool operator!=(array const& a, array const& b) {return not (a == b);};
};

CLASS_HEADER
void 
METHOD_HEADER::build_index()
{
    std::vector<uint64_t> block_rank_pairs;
    uint64_t next_rank = 0;
    uint64_t cur_subrank = 0;
    uint64_t subranks = 0;
    block_rank_pairs.push_back(0);
    for (uint64_t i = 0; i < payload().size(); ++i) {
        uint64_t word_pop = popcount(payload().at(i));
        uint64_t shift = i % super_block_block_size;
        if (shift) { // wait to accumulate the prefix sum of the first block
            subranks <<= 9;
            subranks |= cur_subrank;
        }
        next_rank += word_pop;
        cur_subrank += word_pop;

        if (shift == super_block_block_size - 1) { // only 7 blocks are stored for each header:
            block_rank_pairs.push_back(subranks);  // only 7 blocks are stored for each super-block of 8 blocks, since the last one is taken care of by the sum of the super-block
            block_rank_pairs.push_back(next_rank); // this contains the 8th block
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

    if constexpr (with_select1_hints) {
        std::vector<std::size_t> temp_hints;
        uint64_t cur_ones_threshold = select_ones_per_hint;
        for (std::size_t i = 0; i < super_blocks_size(); ++i) {
            if (super_block_rank1(i + 1) > cur_ones_threshold) {
                temp_hints.push_back(i);
                cur_ones_threshold += select_ones_per_hint;
            }
        }
        temp_hints.push_back(super_blocks_size());
        select1_hints<with_select1_hints>::hints1.swap(temp_hints);
    }

    if constexpr (with_select0_hints) {
        std::vector<std::size_t> temp_hints;
        uint64_t cur_zeros_threshold = select_zeros_per_hint;
        for (std::size_t i = 0; i < super_blocks_size(); ++i) {
            if (super_block_rank0(i + 1) > cur_zeros_threshold) {
                temp_hints.push_back(i);
                cur_zeros_threshold += select_zeros_per_hint;
            }
        }
        temp_hints.push_back(super_blocks_size());
        select0_hints<with_select0_hints>::hints0.swap(temp_hints);
    }
}

CLASS_HEADER
std::size_t 
METHOD_HEADER::rank1(std::size_t idx) const
{
    assert(idx <= size());
    if (idx == size()) return size1();

    std::size_t sub_block = idx / block_bit_size;
    std::size_t r = super_and_block_partial_rank(sub_block);
    std::size_t sub_left = idx % block_bit_size;
    if (sub_left) r += popcount(payload().at(sub_block) << (block_bit_size - sub_left));
    return r;
}

CLASS_HEADER
std::size_t 
METHOD_HEADER::select1(std::size_t th) const
{   //indeces start from 0: e.g. return the 0th 1 = first 1
    assert(th < size1());
    std::size_t a = 0;
    std::size_t b = super_blocks_size();

    if constexpr (with_select1_hints) {
        std::size_t chunk = th / select_ones_per_hint;
        if (chunk != 0) a = select1_hints<with_select1_hints>::hints1.at(chunk - 1);
        b = select1_hints<with_select1_hints>::hints1.at(chunk) + 1;
    }

    while (b - a > 1) {
        std::size_t mid = a + (b - a) / 2;
        std::size_t x = super_block_rank1(mid);
        if (x <= th) a = mid;
        else b = mid;
    }
    auto super_block_idx = a;
    assert(super_block_idx < super_blocks_size());

    std::size_t cur_rank = super_block_rank1(super_block_idx);
    assert(cur_rank <= th);

    std::size_t rank_in_block_parallel = (th - cur_rank) * ones_step_9;
    auto block_rank = block_ranks(super_block_idx);
    uint64_t block_offset = uleq_step_9(block_rank, rank_in_block_parallel) * ones_step_9 >> 54 & 0x7;
    cur_rank += block_rank >> ((7 - block_offset) * 9) & 0x1FF;
    assert(cur_rank <= th);

    uint64_t word_offset = super_block_idx * super_block_block_size + block_offset;
    uint64_t last = payload().at(word_offset);
    std::size_t remaining_bits = th - cur_rank;
    auto local_offset = bit::select1(last, remaining_bits);
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

CLASS_HEADER
std::size_t 
METHOD_HEADER::select0(std::size_t th) const
{
    assert(th < size0());
    std::size_t a = 0;
    std::size_t b = super_blocks_size();

    if constexpr (with_select0_hints) {
        std::size_t chunk = th / select_zeros_per_hint;
        if (chunk != 0) a = select0_hints<with_select0_hints>::hints0.at(chunk - 1);
        b = select0_hints<with_select0_hints>::hints0.at(chunk) + 1;
    }

    while (b - a > 1) {
        std::size_t mid = a + (b - a) / 2;
        std::size_t x = super_block_rank0(mid);
        if (x <= th) a = mid;
        else b = mid;
    }
    auto super_block_idx = a;
    assert(super_block_idx < super_blocks_size());
    std::size_t cur_rank = super_block_rank0(super_block_idx);
    assert(cur_rank <= th);

    // std::cerr << "th = " << th << "\n";
    // std::cerr << "super block idx = " << super_block_idx << "\n";
    // std::cerr << "cur rank = " << cur_rank << "\n";
    
    auto block_rank = block_ranks(super_block_idx);
    // std::cerr << "block rank = " << block_rank << "\n";
    uint64_t block_offset = 0;
    {// we can't use bit-wise tricks since we need to compute the number of zeroes in each packed block.
        std::array<uint16_t, 8> unpacked;
        for (std::size_t i = 7; i != std::numeric_limits<std::size_t>::max(); --i) {
            unpacked[i] = block_bit_size * i - (block_rank & 0x1FF);
            // std::cerr << "unpacked[" << i << "] = " << unpacked.at(i) << "\n";
            block_rank >>= 9;
        }
        while (block_offset < 8 and unpacked.at(block_offset) <= th - cur_rank) {
            // std::cerr << "unpacked[" << block_offset << "] = " << unpacked.at(block_offset) << "\n";
            ++block_offset;
        }
        --block_offset;
        
        // std::cerr << "block offset = " << block_offset << "\n";
        // std::cerr << "delta rank = " << unpacked.at(block_offset) << "\n";
        cur_rank += unpacked.at(block_offset);
    }
    
    assert(cur_rank <= th);

    uint64_t word_offset = super_block_idx * super_block_block_size + block_offset;
    uint64_t last = payload().at(word_offset);
    std::size_t remaining_bits = th - cur_rank;
    // std::cerr << 
    //     "word offset = " << word_offset << 
    //     ", last = " << last << 
    //     ", remaining bits = " << remaining_bits << 
    //     ", local offset = " << local_offset << 
    //     "\n";
    auto local_offset = bit::select0(last, remaining_bits);
    // std::cerr << "local offset = " << local_offset << "\n";
    // std::cerr << "result = " << word_offset * 64 + local_offset << "\n";
    return word_offset * 64 + local_offset;
}

CLASS_HEADER
void 
METHOD_HEADER::swap(array& other) noexcept
{
    _data.swap(other._data);
    interleaved_blocks.swap(other.interleaved_blocks);
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
template <class Visitor>
void 
METHOD_HEADER::visit(Visitor& visitor)
{
    visitor.visit(_data);
    visitor.visit(interleaved_blocks);
    if constexpr (with_select1_hints) visitor.visit(select1_hints<with_select1_hints>::hints1);
    if constexpr (with_select0_hints) visitor.visit(select0_hints<with_select0_hints>::hints0);
}

CLASS_HEADER
template <class Visitor>
void 
METHOD_HEADER::visit(Visitor& visitor) const
{
    visitor.visit(_data);
    visitor.visit(interleaved_blocks);
    if constexpr (with_select1_hints) visitor.visit(select1_hints<with_select1_hints>::hints1);
    if constexpr (with_select0_hints) visitor.visit(select0_hints<with_select0_hints>::hints0);
}

CLASS_HEADER
template <class Loader>
METHOD_HEADER
METHOD_HEADER::load(Loader& visitor)
{
    METHOD_HEADER r;
    // r.visit(visitor);
    r._data = decltype(r._data)::load(visitor);
    visitor.visit(r.interleaved_blocks);
    if constexpr (with_select1_hints) visitor.visit(r.hints1);
    if constexpr (with_select0_hints) visitor.visit(r.hints0);
    return r;
}

#undef CLASS_HEADER
#undef METHOD_HEADER

} // namespace rs
} // namespace bit