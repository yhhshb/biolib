#ifndef PACKED_VECTOR_HPP
#define PACKED_VECTOR_HPP

#include <cstdint>
#include <cstddef>
#include <vector>
#include "bit_operations.hpp"
#include "logtools.hpp"

namespace bit {
namespace packed {

template <typename UnderlyingType = max_width_native_type>
class vector
{
    public:
        class reference
        {
            public:
                reference(const vector* parent, std::size_t pos);

                template <typename IntegerType>
                void operator=(IntegerType const& x);

                template <typename IntegerType>
                explicit operator IntegerType() const;

            private:
                const vector* parent_vector;
                const std::size_t position;
        };

        vector(std::size_t bitwidth);
        vector(vector const&) noexcept = default;
        vector(vector&&) noexcept = default;
        vector& operator=(vector const&) noexcept = default;
        vector& operator=(vector&&) noexcept = default;

        template <typename T>
        void push_back(T val);

        template <typename T>
        T pop_back();

        // template <typename T>
        const reference at(std::size_t index) const;

        // template <typename T>
        reference operator[](std::size_t index);

        template <typename T>
        T front() const;

        template <typename T>
        T back() const;

        std::size_t bit_width() const noexcept;
        UnderlyingType const* data() const noexcept;
        std::vector<UnderlyingType> const& vector_data() const noexcept;
        bool empty() const noexcept;
        std::size_t size() const noexcept;
        std::size_t bit_size() const noexcept;
        std::size_t underlying_size() const noexcept;
        std::size_t max_size() const noexcept;
        void reserve(std::size_t capacity);
        std::size_t capacity() const noexcept;
        void shrink_to_fit() noexcept;
        void resize(std::size_t size);
        void clear() noexcept;
        void swap(vector& other) noexcept;

        template <class Visitor>
        void visit(Visitor& visitor) const;

        template <class Visitor>
        void visit(Visitor& visitor); // for convenience

        template <class Loader>
        static vector load(Loader& visitor);

    private:
        static constexpr std::size_t ut_bit_size = 8 * sizeof(UnderlyingType);
        std::vector<UnderlyingType> _data;
        std::size_t _size;
        std::size_t _bitwidth;
        
        vector();
        std::tuple<std::size_t, long long> index_to_ut_coordinates(std::size_t idx) const noexcept;
        void resize_data(std::size_t size);

        friend bool operator==(vector const& a, vector const& b) 
        {
            bool same_bitwidth = a._bitwidth == b._bitwidth;
            bool same_size = a._size == b._size;
            return same_bitwidth and same_size and (a._data == b._data);
        };
        friend bool operator!=(vector const& a, vector const& b) {return not (a == b);};
};

template <typename UnderlyingType>
vector<UnderlyingType>::vector()
    : _size(0),
      _bitwidth(0)
{
    if (_bitwidth > ut_bit_size) throw std::length_error("[packed vector] objects should fit in the underlying object type");
}

template <typename UnderlyingType>
vector<UnderlyingType>::vector(std::size_t bitwidth)
    : _size(0),
      _bitwidth(bitwidth)
{
    if (_bitwidth > ut_bit_size) throw std::length_error("[packed vector] objects should fit in the underlying object type");
}

template <typename UnderlyingType>
template <typename T>
void
vector<UnderlyingType>::push_back(T val)
{
    assert(val < (static_cast<std::size_t>(1) << _bitwidth));
    // throw std::runtime_error("[packed vector] The value that is being pushed back is wider than the bitwidth");
    resize_data(_size + 1);
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

template <typename UnderlyingType>
template <typename T>
T
vector<UnderlyingType>::pop_back()
{
    T res = at(_size - 1);
    --_size;
    return res;
}

template <typename UnderlyingType>
// template <typename T>
const typename vector<UnderlyingType>::reference 
vector<UnderlyingType>::at(std::size_t index) const
{
    return reference(this, index);
}

template <typename UnderlyingType>
// template <typename T>
typename vector<UnderlyingType>::reference 
vector<UnderlyingType>::operator[](std::size_t index)
{
    return reference(this, index);
}

template <typename UnderlyingType>
template <typename T>
T 
vector<UnderlyingType>::front() const
{
    return static_cast<T>(at(0));
}

template <typename UnderlyingType>
template <typename T>
T 
vector<UnderlyingType>::back() const
{
    return static_cast<T>(at(_size - 1));
}

template <typename UnderlyingType>
std::size_t 
vector<UnderlyingType>::bit_width() const noexcept
{
    return _bitwidth;
}

template <typename UnderlyingType>
UnderlyingType const* 
vector<UnderlyingType>::data() const noexcept
{
    return _data.data();
}

template <typename UnderlyingType>
std::vector<UnderlyingType> const& 
vector<UnderlyingType>::vector_data() const noexcept
{
    return _data;
}

template <typename UnderlyingType>
bool 
vector<UnderlyingType>::empty() const noexcept
{
    return _size == 0;
}

template <typename UnderlyingType>
std::size_t 
vector<UnderlyingType>::size() const noexcept
{
    return _size;
}

template <typename UnderlyingType>
std::size_t 
vector<UnderlyingType>::bit_size() const noexcept
{
    logging_tools::libra l;
    visit(l);
    auto logged_size = l.get_byte_size() * 8;
    return logged_size;
}

template <typename UnderlyingType>
std::size_t 
vector<UnderlyingType>::underlying_size() const noexcept
{
    return _data.size();
}

template <typename UnderlyingType>
std::size_t 
vector<UnderlyingType>::max_size() const noexcept
{
    return _data.max_size() * ut_bit_size / _bitwidth;
}

template <typename UnderlyingType>
void 
vector<UnderlyingType>::reserve(std::size_t capacity)
{
    _data.reserve(capacity * _bitwidth / ut_bit_size + 1);
}

template <typename UnderlyingType>
std::size_t 
vector<UnderlyingType>::capacity() const noexcept
{
    return _data.capacity() * ut_bit_size / _bitwidth;
}

template <typename UnderlyingType>
void 
vector<UnderlyingType>::shrink_to_fit() noexcept
{
    _data.shrink_to_fit();
}

template <typename UnderlyingType>
void 
vector<UnderlyingType>::resize(std::size_t size)
{
    resize_data(size);
    _size = size;
}

template <typename UnderlyingType>
void 
vector<UnderlyingType>::resize_data(std::size_t size)
{
    _data.resize(size * _bitwidth / ut_bit_size + 1);
}

template <typename UnderlyingType>
void 
vector<UnderlyingType>::clear() noexcept
{
    _size = 0;
    _data.clear();
}

template <typename UnderlyingType>
void 
vector<UnderlyingType>::swap(vector& other) noexcept
{
    _data.swap(other._data);
    std::swap(_size, other._size);
    std::swap(_bitwidth, other._bitwidth);
}

template <typename UnderlyingType>
std::tuple<std::size_t, long long> 
vector<UnderlyingType>::index_to_ut_coordinates(std::size_t idx) const noexcept
{
    std::size_t ut_idx = (idx * _bitwidth) / ut_bit_size;
    long long ut_shift = ut_bit_size - _bitwidth - ((idx * _bitwidth) % ut_bit_size);
    return std::make_tuple(ut_idx, ut_shift);
}

template <typename UnderlyingType>
template <class Visitor>
void 
vector<UnderlyingType>::visit(Visitor& visitor)
{
    visitor.visit(_data);
    visitor.visit(_size);
    visitor.visit(_bitwidth);
}

template <typename UnderlyingType>
template <class Visitor>
void 
vector<UnderlyingType>::visit(Visitor& visitor) const
{
    visitor.visit(_data);
    visitor.visit(_size);
    visitor.visit(_bitwidth);
}

template <typename UnderlyingType>
template <class Loader>
vector<UnderlyingType> 
vector<UnderlyingType>::load(Loader& visitor)
{
    vector<UnderlyingType> r;
    r.visit(visitor);
    return r;
}

template <typename UnderlyingType>
vector<UnderlyingType>::reference::reference(const vector<UnderlyingType>* parent, std::size_t pos)
    : parent_vector(parent), position(pos)
{
    if (position >= parent_vector->size()) throw std::out_of_range("[packed vector]: unable to create reference outside storage space");
}

template <typename UnderlyingType>
template <typename IntegerType>
void
vector<UnderlyingType>::reference::operator=(IntegerType const& val)
{
    assert(msbll(val) < parent_vector->bit_width());
    auto [idx, shift] = parent_vector->index_to_ut_coordinates(position);
    if (shift < 0) { // crossing border
        UnderlyingType mask_shift = parent_vector->bit_width() + shift;
        parent_vector->_data[idx] &= ~((UnderlyingType(1) << mask_shift) - 1);
        parent_vector->_data[idx] |= static_cast<UnderlyingType>(val >> -shift);
        parent_vector->_data[idx + 1] &= ((UnderlyingType(1) << (ut_bit_size - parent_vector->bit_width())) - 1);
        parent_vector->_data[idx + 1] |= val << (parent_vector->ut_bit_size - parent_vector->bit_width());
    } else { // perfect fit
        UnderlyingType buffer = val << shift;
        buffer |= parent_vector->_data.at(idx) & ((static_cast<UnderlyingType>(1) << shift) - 1); 
        buffer |= ~(static_cast<UnderlyingType>(1) << ((parent_vector->bit_width() + shift) - 1));
        // if (_bitwidth != ut_bit_size) return buffer & ((T(1) << _bitwidth) - 1);
        // else return buffer;
        parent_vector->_data[idx] = buffer;
    }
}

template <typename UnderlyingType>
template <typename IntegerType>
vector<UnderlyingType>::reference::operator IntegerType() const
{
    // if (index >= _size) throw std::out_of_range("[packed vector]: tried to query element at position " + std::to_string(index) + "(vector size is " + std::to_string(_size) + ")");
    auto [idx, shift] = parent_vector->index_to_ut_coordinates(position);
    // std::cerr << "index = " << index << " : idx = " << idx << ", shift = " << shift << "\n";
    if (shift < 0) { // crossing border
        UnderlyingType buffer = parent_vector->_data.at(idx);
        UnderlyingType mask_shift = parent_vector->bit_width() + shift;
        // std::cerr << "_data[idx] = " << uint64_t(buffer) << "\n";
        // std::cerr << "mask_shift = " << uint64_t(mask_shift) << "\n";
        buffer &= (UnderlyingType(1) << mask_shift) - 1;
        buffer <<= -shift;
        // auto buffer2 = _data.at(idx + 1) & ~((UnderlyingType(1) << (ut_bit_size - _bitwidth)) - 1);
        // buffer |= buffer2 >> (ut_bit_size + shift); // buffer2 was used only for debugging
        // std::cerr << "left part = " << uint64_t(buffer) << "\n";
        // std::cerr << "right part = " << uint64_t(buffer2) << "\n";
        buffer |= parent_vector->_data.at(idx + 1) >> (parent_vector->ut_bit_size + shift);
        return static_cast<IntegerType>(buffer);
    } else { // perfect fit
        UnderlyingType buffer = parent_vector->_data.at(idx) >> shift;
        if (parent_vector->bit_width() != parent_vector->ut_bit_size) {
            buffer &= ((static_cast<UnderlyingType>(1) << parent_vector->bit_width()) - 1);
        }
        return static_cast<IntegerType>(buffer);
    }
}

} // namespace packed
} // namespace bit

#endif // PACKED_VECTOR_HPP