#ifndef PACKED_VECTOR_HPP
#define PACKED_VECTOR_HPP

#include <cstdint>
#include <cstddef>
#include <vector>
#include "bit_operations.hpp"

namespace bit {
namespace packed {

template <std::size_t bitwidth, typename UnderlyingType = max_width_native_type>
class vector
{
    public:
        vector();

        vector(vector const&) = default;

        constexpr vector(vector&&) noexcept = default;

        template <typename T>
        void push_back(T val);

        template <typename T>
        T pop_back();

        template <typename T>
        T at(std::size_t index) const;

        template <typename T>
        T operator[](std::size_t index) const;

        template <typename T>
        T front() const;

        template <typename T>
        T back() const;

        UnderlyingType const* data() const noexcept;
        std::vector<UnderlyingType> const& vector_data() const noexcept;
        bool empty() const noexcept;
        std::size_t size() const noexcept;
        std::size_t max_size() const noexcept;
        void reserve(std::size_t capacity);
        std::size_t capacity() const noexcept;
        void shrink_to_fit() noexcept;
        void resize(std::size_t size);
        void clear() noexcept;
        void swap(vector& other);

        template <class Visitor>
        void visit(Visitor& visitor) const;
    
    private:
        static constexpr std::size_t ut_bit_size = 8 * sizeof(UnderlyingType);
        std::vector<UnderlyingType> _data;
        const std::size_t ut_overhead;
        std::size_t _size;

        std::tuple<std::size_t, long long> index_to_ut_coordinates(std::size_t idx) const noexcept;
};

template <std::size_t bitwidth, typename UnderlyingType>
vector<bitwidth, UnderlyingType>::vector()
    : ut_overhead(static_cast<std::size_t>(std::ceil(bitwidth / ut_bit_size))), _size(0)
{
    static_assert(bitwidth <= ut_bit_size, "[packed vector] objects should fit in the underlying aobject type");
    if constexpr (bitwidth > ut_bit_size) throw std::length_error("[packed vector] objects should fit in the underlying aobject type");
}

template <std::size_t bitwidth, typename UnderlyingType>
template <typename T>
void
vector<bitwidth, UnderlyingType>::push_back(T val)
{
    resize(_size + 1);
    auto [idx, shift] = index_to_ut_coordinates(_size);
    // std::cerr << idx + ut_overhead + 1 << "\n";
    // std::cerr << "_size = " << _size << std::endl;
    // std::cerr << "data size = " << _data.size() << std::endl;
    UnderlyingType buffer = static_cast<UnderlyingType>(val);
    if (shift < 0) { // crossing border
        UnderlyingType buffer2 = buffer;
        buffer >>= -shift;
        buffer2 <<= ut_bit_size + shift;
        // std::cerr << "------ " << idx + 1 << std::endl;
        _data[idx + 1] |= buffer2; // NOTE +1 works, since UnderlyingType is wide enough to contain at least one element (see constructor)
    } else { // nicely fits into a single UnderlyingType cell
        buffer <<= shift;
    }
    _data[idx] |= buffer;
    ++_size;
}

template <std::size_t bitwidth, typename UnderlyingType>
template <typename T>
T
vector<bitwidth, UnderlyingType>::pop_back()
{
    T res = at(_size - 1);
    --_size;
    return res;
}

template <std::size_t bitwidth, typename UnderlyingType>
template <typename T>
T 
vector<bitwidth, UnderlyingType>::at(std::size_t index) const
{
    if (index >= _size) throw std::out_of_range("[packed vector]: tried to query element at position " + std::to_string(index) + "(vector size is " + std::to_string(_size) + ")");
    auto [idx, shift] = index_to_ut_coordinates(index);
    // std::cerr << "index = " << index << " : idx = " << idx << ", shift = " << shift << "\n";
    if (shift < 0) { // crossing border
        UnderlyingType buffer = _data[idx];
        // std::cerr << "_data[idx] = " << uint64_t(buffer) << "\n";
        // std::size_t mask_shift = ut_bit_size + shift + bitwidth;
        UnderlyingType mask_shift = bitwidth + shift;
        // std::cerr << "mask_shift = " << uint64_t(mask_shift) << "\n";
        buffer &= (UnderlyingType(1) << mask_shift) - 1;
        buffer <<= -shift;
        // std::cerr << "left part = " << uint64_t(buffer) << "\n";
        auto buffer2 = _data[idx + 1] & ~((UnderlyingType(1) << (ut_bit_size - bitwidth)) - 1);
        // std::cerr << "right part = " << uint64_t(buffer2) << "\n";
        buffer |= buffer2 >> (ut_bit_size + shift);
        return static_cast<T>(buffer);
    } else { // perfect fit
        UnderlyingType buffer = static_cast<T>(_data[idx] >> shift);
        if (bitwidth != ut_bit_size) return buffer & ((T(1) << bitwidth) - 1);
        else return buffer;
    }
}

template <std::size_t bitwidth, typename UnderlyingType>
template <typename T>
T 
vector<bitwidth, UnderlyingType>::operator[](std::size_t index) const
{
    at(index);
}

template <std::size_t bitwidth, typename UnderlyingType>
template <typename T>
T 
vector<bitwidth, UnderlyingType>::front() const
{
    return at(0);
}

template <std::size_t bitwidth, typename UnderlyingType>
template <typename T>
T 
vector<bitwidth, UnderlyingType>::back() const
{
    return at(_size - 1);
}

template <std::size_t bitwidth, typename UnderlyingType>
UnderlyingType const* 
vector<bitwidth, UnderlyingType>::data() const noexcept
{
    return _data.data();
}

template <std::size_t bitwidth, typename UnderlyingType>
std::vector<UnderlyingType> const& 
vector<bitwidth, UnderlyingType>::vector_data() const noexcept
{
    return _data;
}

template <std::size_t bitwidth, typename UnderlyingType>
bool 
vector<bitwidth, UnderlyingType>::empty() const noexcept
{
    return _size == 0;
}

template <std::size_t bitwidth, typename UnderlyingType>
std::size_t 
vector<bitwidth, UnderlyingType>::size() const noexcept
{
    return _size;
}

template <std::size_t bitwidth, typename UnderlyingType>
std::size_t 
vector<bitwidth, UnderlyingType>::max_size() const noexcept
{
    return _data.max_size() * ut_bit_size / bitwidth;
}

template <std::size_t bitwidth, typename UnderlyingType>
void 
vector<bitwidth, UnderlyingType>::reserve(std::size_t capacity)
{
    _data.reserve(capacity * bitwidth / ut_bit_size + 1);
}

template <std::size_t bitwidth, typename UnderlyingType>
std::size_t 
vector<bitwidth, UnderlyingType>::capacity() const noexcept
{
    return _data.capacity() * ut_bit_size / bitwidth;
}

template <std::size_t bitwidth, typename UnderlyingType>
void 
vector<bitwidth, UnderlyingType>::shrink_to_fit() noexcept
{
    _data.shrink_to_fit();
}

template <std::size_t bitwidth, typename UnderlyingType>
void 
vector<bitwidth, UnderlyingType>::resize(std::size_t size)
{
    _data.resize(size * bitwidth / ut_bit_size + 1);
}

template <std::size_t bitwidth, typename UnderlyingType>
void 
vector<bitwidth, UnderlyingType>::clear() noexcept
{
    _size = 0;
    _data.clear();
}

template <std::size_t bitwidth, typename UnderlyingType>
void 
vector<bitwidth, UnderlyingType>::swap(vector& other)
{
    std::swap(_size, other._size);
    _data.swap(other._data);
}

template <std::size_t bitwidth, typename UnderlyingType>
std::tuple<std::size_t, long long> 
vector<bitwidth, UnderlyingType>::index_to_ut_coordinates(std::size_t idx) const noexcept
{
    // if (idx >= _size) throw std::out_of_range("[packed vector]");
    std::size_t ut_idx = (idx * bitwidth) / ut_bit_size;
    long long ut_shift = ut_bit_size - bitwidth - ((idx * bitwidth) % ut_bit_size);
    return std::make_tuple(ut_idx, ut_shift);
}

template <std::size_t bitwidth, typename UnderlyingType>
template <class Visitor>
void 
vector<bitwidth, UnderlyingType>::visit(Visitor& visitor) const
{
    visitor.apply(_data);
    visitor.apply(ut_overhead);
    visitor.apply(_size);
}

} // namespace packed
} // namespace bit

#endif // PACKED_VECTOR_HPP