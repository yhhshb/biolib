#ifndef PACKED_VECTOR_HPP
#define PACKED_VECTOR_HPP

#include <cstdint>
#include <cstddef>
#include <vector>
#include "bit_operations.hpp"
#include "logtools.hpp"

#define CLASS_HEADER template <typename UnderlyingType>
#define METHOD_HEADER vector<UnderlyingType>

namespace bit {
namespace packed {

template <typename UnderlyingType = max_width_native_type>
class vector
{
    public:
        class const_reference
        {
            public:
                const_reference(vector const * const parent, std::size_t pos);

                template <typename IntegerType>
                explicit operator IntegerType() const;

            private:
                vector const * const parent_vector;
                const std::size_t position;
        };

        class reference
        {
            public:
                reference(vector *const parent, std::size_t pos);

                template <typename IntegerType>
                void operator=(IntegerType const& x);

                template <typename IntegerType>
                explicit operator IntegerType() const;

            private:
                vector *const parent_vector;
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

        const_reference at(std::size_t index) const;
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

CLASS_HEADER
METHOD_HEADER::vector()
    : _size(0),
      _bitwidth(0)
{
    if (_bitwidth > ut_bit_size) throw std::length_error("[packed vector] objects should fit in the underlying object type");
}

CLASS_HEADER
METHOD_HEADER::vector(std::size_t bitwidth)
    : _size(0),
      _bitwidth(bitwidth)
{
    if (_bitwidth > ut_bit_size) throw std::length_error("[packed vector] objects should fit in the underlying object type");
}

CLASS_HEADER
template <typename T>
void
METHOD_HEADER::push_back(T val)
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

CLASS_HEADER
template <typename T>
T
METHOD_HEADER::pop_back()
{
    T res = at(_size - 1);
    --_size;
    return res;
}

CLASS_HEADER
// template <typename T>
typename vector<UnderlyingType>::const_reference 
METHOD_HEADER::at(std::size_t index) const
{
    return const_reference(this, index);
}

CLASS_HEADER
// template <typename T>
typename vector<UnderlyingType>::reference 
METHOD_HEADER::operator[](std::size_t index)
{
    return reference(this, index);
}

CLASS_HEADER
template <typename T>
T 
METHOD_HEADER::front() const
{
    return static_cast<T>(at(0));
}

CLASS_HEADER
template <typename T>
T 
METHOD_HEADER::back() const
{
    return static_cast<T>(at(_size - 1));
}

CLASS_HEADER
std::size_t 
METHOD_HEADER::bit_width() const noexcept
{
    return _bitwidth;
}

CLASS_HEADER
UnderlyingType const* 
METHOD_HEADER::data() const noexcept
{
    return _data.data();
}

CLASS_HEADER
std::vector<UnderlyingType> const& 
METHOD_HEADER::vector_data() const noexcept
{
    return _data;
}

CLASS_HEADER
bool 
METHOD_HEADER::empty() const noexcept
{
    return _size == 0;
}

CLASS_HEADER
std::size_t 
METHOD_HEADER::size() const noexcept
{
    return _size;
}

CLASS_HEADER
std::size_t 
METHOD_HEADER::bit_size() const noexcept
{
    logging_tools::libra l;
    visit(l);
    auto logged_size = l.get_byte_size() * 8;
    return logged_size;
}

CLASS_HEADER
std::size_t 
METHOD_HEADER::underlying_size() const noexcept
{
    return _data.size();
}

CLASS_HEADER
std::size_t 
METHOD_HEADER::max_size() const noexcept
{
    return _data.max_size() * ut_bit_size / _bitwidth;
}

CLASS_HEADER
void 
METHOD_HEADER::reserve(std::size_t capacity)
{
    _data.reserve(capacity * _bitwidth / ut_bit_size + 1);
}

CLASS_HEADER
std::size_t 
METHOD_HEADER::capacity() const noexcept
{
    return _data.capacity() * ut_bit_size / _bitwidth;
}

CLASS_HEADER
void 
METHOD_HEADER::shrink_to_fit() noexcept
{
    _data.shrink_to_fit();
}

CLASS_HEADER
void 
METHOD_HEADER::resize(std::size_t size)
{
    resize_data(size);
    _size = size;
}

CLASS_HEADER
void 
METHOD_HEADER::resize_data(std::size_t size)
{
    _data.resize(size * _bitwidth / ut_bit_size + 1);
}

CLASS_HEADER
void 
METHOD_HEADER::clear() noexcept
{
    _size = 0;
    _data.clear();
}

CLASS_HEADER
void 
METHOD_HEADER::swap(vector& other) noexcept
{
    _data.swap(other._data);
    std::swap(_size, other._size);
    std::swap(_bitwidth, other._bitwidth);
}

CLASS_HEADER
std::tuple<std::size_t, long long> 
METHOD_HEADER::index_to_ut_coordinates(std::size_t idx) const noexcept
{
    std::size_t ut_idx = (idx * _bitwidth) / ut_bit_size;
    long long ut_shift = ut_bit_size - _bitwidth - ((idx * _bitwidth) % ut_bit_size);
    return std::make_tuple(ut_idx, ut_shift);
}

CLASS_HEADER
template <class Visitor>
void 
METHOD_HEADER::visit(Visitor& visitor)
{
    visitor.visit(_data);
    visitor.visit(_size);
    visitor.visit(_bitwidth);
}

CLASS_HEADER
template <class Visitor>
void 
METHOD_HEADER::visit(Visitor& visitor) const
{
    visitor.visit(_data);
    visitor.visit(_size);
    visitor.visit(_bitwidth);
}

CLASS_HEADER
template <class Loader>
vector<UnderlyingType> 
METHOD_HEADER::load(Loader& visitor)
{
    vector<UnderlyingType> r;
    r.visit(visitor);
    return r;
}

CLASS_HEADER
METHOD_HEADER::reference::reference(vector<UnderlyingType> *const parent, std::size_t pos)
    : parent_vector(parent), position(pos)
{
    if (position >= parent_vector->size()) throw std::out_of_range("[packed vector]: unable to create reference outside storage space");
}

CLASS_HEADER
template <typename IntegerType>
void
METHOD_HEADER::reference::operator=(IntegerType const& val)
{
    // std::cerr << "--------------- operator= ------------------\n";
    assert(val == 0 or msbll(val) < parent_vector->bit_width());
    auto [idx, shift] = parent_vector->index_to_ut_coordinates(position);
    // std::cerr << "index = " << index << " : idx = " << idx << ", shift = " << shift << "\n";
    if (shift < 0) { // crossing border
        UnderlyingType mask_shift = parent_vector->bit_width() + shift;
        parent_vector->_data[idx] &= ~((UnderlyingType(1) << mask_shift) - 1);
        parent_vector->_data[idx] |= static_cast<UnderlyingType>(val >> -shift);
        // parent_vector->_data[idx + 1] &= ((UnderlyingType(1) << (ut_bit_size - parent_vector->bit_width())) - 1);
        // parent_vector->_data[idx + 1] |= val << (parent_vector->ut_bit_size - parent_vector->bit_width());
        parent_vector->_data[idx + 1] &= ((UnderlyingType(1) << (ut_bit_size + shift)) - 1);
        parent_vector->_data[idx + 1] |= val << (ut_bit_size + shift);
    } else { // perfect fit
        // std::cerr << "data at(" << idx << ") = " << static_cast<std::size_t>(parent_vector->_data.at(idx)) << "\n";
        UnderlyingType buffer = val << shift;
        // std::cerr << "buffer : " << static_cast<std::size_t>(buffer) << "\n";
        buffer |= parent_vector->_data.at(idx) & ((static_cast<UnderlyingType>(1) << shift) - 1); 
        // std::cerr << static_cast<std::size_t>(buffer) << "\n";
        if (parent_vector->_bitwidth + shift != parent_vector->ut_bit_size) {
            buffer |= parent_vector->_data.at(idx) & ~((static_cast<UnderlyingType>(1) << (parent_vector->bit_width() + shift)) - 1);
        }
        // std::cerr << static_cast<std::size_t>(buffer) << "\n";
        parent_vector->_data[idx] = buffer;
    }
    // std::cerr << "**************************************************\n";
}

CLASS_HEADER
template <typename IntegerType>
METHOD_HEADER::reference::operator IntegerType() const
{
    // std::cerr << "--------------- operator cast ----------------\n";
    auto [idx, shift] = parent_vector->index_to_ut_coordinates(position);
    // std::cerr << "index = " << index << " : idx = " << idx << ", shift = " << shift << "\n";
    if (shift < 0) { // crossing border
        UnderlyingType buffer = parent_vector->_data.at(idx);
        UnderlyingType mask_shift = parent_vector->bit_width() + shift;
        // std::cerr << "_data[idx] = " << uint64_t(buffer) << "\n";
        // std::cerr << "mask_shift = " << uint64_t(mask_shift) << "\n";
        buffer &= (UnderlyingType(1) << mask_shift) - 1;
        buffer <<= -shift;
        // auto buffer2 = parent_vector->_data.at(idx + 1) & ~((UnderlyingType(1) << (ut_bit_size - parent_vector->_bitwidth)) - 1);
        // buffer |= buffer2 >> (ut_bit_size + shift); // buffer2 was used only for debugging
        // std::cerr << "left part = " << uint64_t(buffer) << "\n";
        // std::cerr << "right part = " << uint64_t(buffer2) << "\n";
        buffer |= parent_vector->_data.at(idx + 1) >> (parent_vector->ut_bit_size + shift);
        return static_cast<IntegerType>(buffer);
    } else { // perfect fit
        // std::cerr << "data at(" << idx << ") = " << static_cast<std::size_t>(parent_vector->_data.at(idx)) << "\n";
        UnderlyingType buffer = parent_vector->_data.at(idx) >> shift;
        // std::cerr << static_cast<std::size_t>(buffer) << "\n";
        if (parent_vector->_bitwidth != parent_vector->ut_bit_size) { // mask if it does not exactly fits the underlying type
            buffer &= ((static_cast<UnderlyingType>(1) << parent_vector->bit_width())) - 1;
        }
        // std::cerr << static_cast<std::size_t>(buffer) << "\n";
        return static_cast<IntegerType>(buffer);
    }
    // std::cerr << "**************************************************\n";
}

CLASS_HEADER
METHOD_HEADER::const_reference::const_reference(vector<UnderlyingType> const * const parent, std::size_t pos)
    : parent_vector(parent), position(pos)
{
    if (position >= parent_vector->size()) throw std::out_of_range("[packed vector]: unable to create reference outside storage space");
}

CLASS_HEADER
template <typename IntegerType>
METHOD_HEADER::const_reference::operator IntegerType() const
{
    auto [idx, shift] = parent_vector->index_to_ut_coordinates(position);
    if (shift < 0) { // crossing border
        UnderlyingType buffer = parent_vector->_data.at(idx);
        UnderlyingType mask_shift = parent_vector->bit_width() + shift;
        buffer &= (UnderlyingType(1) << mask_shift) - 1;
        buffer <<= -shift;
        buffer |= parent_vector->_data.at(idx + 1) >> (parent_vector->ut_bit_size + shift);
        return static_cast<IntegerType>(buffer);
    } else { // perfect fit
        UnderlyingType buffer = parent_vector->_data.at(idx) >> shift;
        if (parent_vector->_bitwidth != parent_vector->ut_bit_size) { // mask if it does not exactly fits the underlying type
            buffer &= ((static_cast<UnderlyingType>(1) << parent_vector->bit_width())) - 1;
        }
        return static_cast<IntegerType>(buffer);
    }
}

#undef METHOD_HEADER
#undef CLASS_HEADER

} // namespace packed
} // namespace bit

#endif // PACKED_VECTOR_HPP