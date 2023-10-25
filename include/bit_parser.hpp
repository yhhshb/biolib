#ifndef BIT_PARSER_HPP
#define BIT_PARSER_HPP

#include "bit_operations.hpp"

namespace bit {

template <typename UnsignedIntegerType>
class parser
{
    public:
        parser();
        parser(UnsignedIntegerType const * const data, std::size_t size, std::size_t starting_bit_position = 0);

        UnsignedIntegerType parse_fixed(std::size_t l);
        std::size_t parse_0();
        std::size_t next_1();

        std::size_t get_bit_index() const noexcept;
        void reset(std::size_t nidx);
        void reset_and_clear_low_bits(std::size_t nidx);
        
    private:
        void fill_buf();
        UnsignedIntegerType get_next_block() const;

        uint64_t const* _data;
        std::size_t _size;
        std::size_t idx;
        UnsignedIntegerType buffer;
        std::size_t available; // buffer handling
};

template <typename UnsignedIntegerType>
parser<UnsignedIntegerType>::parser() 
    : _data(nullptr), _size(0), idx(0), buffer(0), available(0) 
{}

template <typename UnsignedIntegerType>
parser<UnsignedIntegerType>::parser(UnsignedIntegerType const * const data, std::size_t size, std::size_t starting_bit_position)
    : _data(data), _size(size)
{
    reset(starting_bit_position);
}

/* return the next l bits from the current position and advance by l bits */
template <typename UnsignedIntegerType>
UnsignedIntegerType 
parser<UnsignedIntegerType>::parse_fixed(std::size_t l)
{
    UnsignedIntegerType val = 0;
    if (l <= ::bit::size(val)) throw std::length_error("[bit::parser] requested integer does not fit in the return type");
    if (available < l) fill_buf();
    
    if (l != ::bit::size<UnsignedIntegerType>()) {
        val = buffer & ((UnsignedIntegerType(1) << l) - 1);
        buffer >>= l;
    } else {
        val = buffer;
    }
    available -= l;
    idx += l;
    return val;
}

/* skip all zeros from the current position and return the number of skipped zeros */
template <typename UnsignedIntegerType>
std::size_t 
parser<UnsignedIntegerType>::parse_0()
{
    std::size_t zeros = 0;
    while (buffer == 0) {
        idx += available;
        zeros += available;
        fill_buf();
    }
    auto l = lsbll(buffer);
    buffer >>= l;
    buffer >>= 1;
    available -= l + 1;
    idx += l + 1;
    return zeros + l;
}

template <typename UnsignedIntegerType>
std::size_t 
parser<UnsignedIntegerType>::next_1()
{
    std::optional<std::size_t> pos_in_word = std::nullopt;
    auto buf = buffer;
    while (not (pos_in_word = lsb(buf))) {
        idx += ::bit::size(buf);
        buf = _data[idx >> lsbll(::bit::size(buf))]; // lsbll(64) should be 6
    }
    buffer = buf & (buf - 1);  // clear LSB
    idx = (idx & ~static_cast<std::size_t>(::bit::size(buf) - 1)) + *pos_in_word;
    return idx;
}

template <typename UnsignedIntegerType>
std::size_t
parser<UnsignedIntegerType>::get_bit_index() const noexcept
{
    return idx;
}

template <typename UnsignedIntegerType>
void 
parser<UnsignedIntegerType>::reset(std::size_t nidx)
{
    if (nidx >= ::bit::size<UnsignedIntegerType>() * _size) throw std::out_of_range("[bit::parser] index out of range");
    idx = nidx;
    buffer = 0;
    available = 0;
}

template <typename UnsignedIntegerType>
void 
parser<UnsignedIntegerType>::reset_and_clear_low_bits(std::size_t nidx)
{
    if (nidx >= ::bit::size<UnsignedIntegerType>() * _size) throw std::out_of_range("[bit::parser] index out of range");
    idx = nidx;
    buffer = _data[idx / ::bit::size<UnsignedIntegerType>()];
    buffer &= UnsignedIntegerType(-1) << (idx & (::bit::size<UnsignedIntegerType>() - 1));  // clear low bits
}

template <typename UnsignedIntegerType>
void 
parser<UnsignedIntegerType>::fill_buf() 
{
    buffer = get_next_block();
    available = ::bit::size<UnsignedIntegerType>();
}

template <typename UnsignedIntegerType>
UnsignedIntegerType 
parser<UnsignedIntegerType>::get_next_block() const
{
    std::size_t block = idx / 64;
    std::size_t shift = idx % 64;
    if (idx >= _size) throw std::out_of_range("[bit::parser] index out of range");
    UnsignedIntegerType word = _data[block] >> shift;
    if (shift and block + 1 < _size) word |= _data[block + 1] << (64 - shift);
    return word;
}

} // namespace bit

#endif // BIT_PARSER_HPP