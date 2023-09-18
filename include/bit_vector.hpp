#ifndef BIT_HPP
#define BIT_HPP

#include <vector>
#include <tuple>
#include <stdexcept>
#include <cassert>

#include "bit_operations.hpp"

namespace bit {

template <typename UnsignedIntegerType>
class vector
{
    public:
        using value_type = UnsignedIntegerType;
        using block_type = UnsignedIntegerType;

        class const_iterator // iterator over all bits (true or false)
        {
            public:
                using iterator_category = std::forward_iterator_tag;
                using difference_type   = std::ptrdiff_t;
                using value_type        = bool;
                using pointer           = value_type*;
                using reference         = value_type&;

                const_iterator(vector const& vec, std::size_t ref_idx);
                bool operator*() const;
                const_iterator const& operator++() noexcept;
                const_iterator operator++(int) noexcept;

                template <typename I>
                const_iterator operator+(I) const;
            
            private:
                vector const& parent_vector;
                std::size_t idx;
                UnsignedIntegerType buffer;

                friend bool operator==(const_iterator const& a, const_iterator const& b) 
                {
                    bool same_start = a.idx == b.idx;
                    return (&a.parent_vector == &b.parent_vector) and same_start;
                };
                friend bool operator!=(const_iterator const& a, const_iterator const& b) {return not (a == b);};
        };

        class one_position_iterator // iterator over select positions
        {
            public:
                using iterator_category = std::forward_iterator_tag;
                using difference_type   = std::ptrdiff_t;
                using value_type        = std::size_t;
                using pointer           = value_type*;
                using reference         = value_type&;

                one_position_iterator(vector const& vec, std::size_t ref_idx);
                std::size_t operator*() const;
                one_position_iterator const& operator++() noexcept;
                one_position_iterator operator++(int) noexcept;

                template <typename I>
                one_position_iterator operator+(I delta_on_the_actual_sequence_of_bits) noexcept;
            
            private:
                vector const& parent_vector;
                std::size_t idx;

                void find_next_one() noexcept;

                friend bool operator==(const_iterator const& a, const_iterator const& b) 
                {
                    bool same_start = a.idx == b.idx;
                    return (&a.parent_vector == &b.parent_vector) and same_start;
                };
                friend bool operator!=(const_iterator const& a, const_iterator const& b) {return not (a == b);};
        };

        vector() noexcept;
        vector(std::size_t size, bool val = false);
        vector(vector const&) noexcept = default;
        vector(vector&&) noexcept = default;
        vector& operator=(vector const&) noexcept = default;
        vector& operator=(vector&&) noexcept = default;
        void reserve(std::size_t capacity);
        void resize(std::size_t size);
        void resize(std::size_t size, bool value);
        void clear();

        void push_back(bool bit);
        void push_back(UnsignedIntegerType block, std::size_t suffix_len);
        bool pop_back();

        void set(std::size_t idx);
        void clear(std::size_t idx);
        bool at(std::size_t idx) const;
        bool front() const;
        bool back() const;
        bool empty() const noexcept;

        UnsignedIntegerType& operator[](std::size_t idx);
        UnsignedIntegerType const& block_at(std::size_t idx) const;

        const_iterator cbegin() const;
        const_iterator cend() const;
        const_iterator begin() const {return cbegin();}
        const_iterator end() const {return cend();}

        one_position_iterator cpos_begin() const;
        one_position_iterator cpos_end() const;
        one_position_iterator pos_begin() const {return cpos_begin();}
        one_position_iterator pos_end() const {return cpos_end();}

        UnsignedIntegerType const* data() const noexcept;
        std::vector<UnsignedIntegerType> const& vector_data() const noexcept;
        std::size_t block_size() const noexcept;
        std::size_t size() const noexcept;
        std::size_t bit_size() const noexcept;
        std::size_t capacity() const noexcept;
        void shrink_to_fit() noexcept;
        std::size_t max_size() const noexcept;
        void swap(vector& other);

        template <class Visitor>
        void visit(Visitor& visitor) const;

        template <class Visitor>
        void visit(Visitor& visitor); // here for convenience but in normal circumstances load should be used instead

        template <class Loader>
        static vector load(Loader& visitor);
        
    private:
        static constexpr std::size_t block_bit_size = 8 * sizeof(UnsignedIntegerType);
        std::vector<UnsignedIntegerType> _data;
        std::size_t bsize;

        std::size_t bit_to_byte_size(std::size_t bit_size) const noexcept;
        std::tuple<std::size_t, std::size_t> idx_to_coordinates(std::size_t idx) const noexcept;
        void check_coordinates(std::size_t idx) const;

        friend bool operator==(vector const& a, vector const& b) 
        {
            bool same_size = a.bsize == b.bsize;
            return same_size and (a._data == b._data);
        };
        friend bool operator!=(vector const& a, vector const& b) {return not (a == b);};
};

template <typename UnsignedIntegerType>
vector<UnsignedIntegerType>::vector() noexcept 
    : bsize(0)
{}

template <typename UnsignedIntegerType>
vector<UnsignedIntegerType>::vector(std::size_t size, bool val) 
    : bsize(size)
{
    _data.resize(bit_to_byte_size(bsize), static_cast<UnsignedIntegerType>(val));
}

template <typename UnsignedIntegerType>
void 
vector<UnsignedIntegerType>::reserve(std::size_t capacity) 
{
    _data.reserve(bit_to_byte_size(capacity));
}

template <typename UnsignedIntegerType>
void 
vector<UnsignedIntegerType>::resize(std::size_t size) 
{
    _data.resize(bit_to_byte_size(size));
    bsize = size;
}

template <typename UnsignedIntegerType>
void 
vector<UnsignedIntegerType>::resize(std::size_t size, bool value) 
{
    _data.resize(
        bit_to_byte_size(size), 
        value ? 
            static_cast<UnsignedIntegerType>(0) - static_cast<UnsignedIntegerType>(1) 
            : static_cast<UnsignedIntegerType>(0)
    );
    bsize = size;
}

template <typename UnsignedIntegerType>
void 
vector<UnsignedIntegerType>::clear() 
{
    _data.clear();
    bsize = 0;
}

template <typename UnsignedIntegerType>
void 
vector<UnsignedIntegerType>::push_back(bool bit)
{
    auto [a, b] = idx_to_coordinates(bsize);
    if (a == _data.size()) _data.push_back(static_cast<block_type>(0));
    assert(_data.size() > a);
    _data[a] |= static_cast<block_type>(bit) << (b);
    ++bsize;
    // auto [block_idx, bit_idx] = idx_to_coordinates(bsize);
}

template <typename UnsignedIntegerType>
void 
vector<UnsignedIntegerType>::push_back(UnsignedIntegerType block, std::size_t suffix_len)
{
    assert(suffix_len <= ::bit::size(block));
    auto [a, b] = idx_to_coordinates(bsize + suffix_len);
    if (a >= _data.size()) {
        _data.push_back(static_cast<block_type>(block >> (suffix_len - b))); // insert msb directly into new block
    }
    assert(_data.size() > a or suffix_len == block_bit_size);
    auto [block_idx, bit_idx] = idx_to_coordinates(bsize);
    _data[block_idx] |= block << bit_idx; // insert lsb into (old) last block
    bsize += suffix_len;
}

template <typename UnsignedIntegerType>
bool 
vector<UnsignedIntegerType>::pop_back()
{
    if (bsize == 0) throw std::out_of_range("[bit::vector::pop_back]");
    bool res = at(bsize - 1);
    --bsize;
    return res;
}

template <typename UnsignedIntegerType>
void 
vector<UnsignedIntegerType>::set(std::size_t idx)
{
    check_coordinates(idx);
    auto [block_idx, bit_idx] = idx_to_coordinates(idx);
    _data[block_idx] |= static_cast<UnsignedIntegerType>(1) << bit_idx;
}

template <typename UnsignedIntegerType>
void 
vector<UnsignedIntegerType>::clear(std::size_t idx)
{
    check_coordinates(idx);
    auto [block_idx, bit_idx] = idx_to_coordinates(idx);
    _data[block_idx] &= ~(static_cast<UnsignedIntegerType>(1) << bit_idx);
}

template <typename UnsignedIntegerType>
bool 
vector<UnsignedIntegerType>::at(std::size_t idx) const
{
    check_coordinates(idx);
    auto [block_idx, bit_idx] = idx_to_coordinates(idx);
    return static_cast<bool>(_data.at(block_idx) & (static_cast<UnsignedIntegerType>(1) << bit_idx));
}

template <typename UnsignedIntegerType>
bool 
vector<UnsignedIntegerType>::front() const
{
    return at(0);
}

template <typename UnsignedIntegerType>
bool 
vector<UnsignedIntegerType>::back() const
{
    return at(bsize-1);
}

template <typename UnsignedIntegerType>
bool 
vector<UnsignedIntegerType>::empty() const noexcept
{
    return bsize == 0;
}

template <typename UnsignedIntegerType>
UnsignedIntegerType&
vector<UnsignedIntegerType>::operator[](std::size_t idx)
{
    check_coordinates(idx);
    auto [block_idx, bit_idx] = idx_to_coordinates(idx);
    return _data[block_idx];
}

template <typename UnsignedIntegerType>
UnsignedIntegerType const&
vector<UnsignedIntegerType>::block_at(std::size_t idx) const
{
    check_coordinates(idx);
    auto [block_idx, bit_idx] = idx_to_coordinates(idx);
    return _data.at(block_idx);
}

template <typename UnsignedIntegerType>
typename vector<UnsignedIntegerType>::const_iterator
vector<UnsignedIntegerType>::cbegin() const
{
    return const_iterator(*this, 0);
}

template <typename UnsignedIntegerType>
typename vector<UnsignedIntegerType>::const_iterator
vector<UnsignedIntegerType>::cend() const
{
    return const_iterator(*this, bsize);
}

template <typename UnsignedIntegerType>
typename vector<UnsignedIntegerType>::one_position_iterator
vector<UnsignedIntegerType>::cpos_begin() const
{
    return one_position_iterator(*this, 0);
}

template <typename UnsignedIntegerType>
typename vector<UnsignedIntegerType>::one_position_iterator
vector<UnsignedIntegerType>::cpos_end() const
{
    return one_position_iterator(*this, bsize);
}

template <typename UnsignedIntegerType>
UnsignedIntegerType const* 
vector<UnsignedIntegerType>::data() const noexcept
{
    return _data.data();
}

template <typename UnsignedIntegerType>
std::vector<UnsignedIntegerType> const& 
vector<UnsignedIntegerType>::vector_data() const noexcept
{
    return _data;
}

template <typename UnsignedIntegerType>
std::size_t
vector<UnsignedIntegerType>::block_size() const noexcept 
{ 
    assert(bit_to_byte_size(bsize) == _data.size());
    return _data.size();
}

template <typename UnsignedIntegerType>
std::size_t
vector<UnsignedIntegerType>::size() const noexcept 
{
    return bsize;
}

template <typename UnsignedIntegerType>
std::size_t
vector<UnsignedIntegerType>::bit_size() const noexcept 
{
    return block_size() * sizeof(UnsignedIntegerType) * 8; // Do not use logtools for maximum portability
}

template <typename UnsignedIntegerType>
std::size_t 
vector<UnsignedIntegerType>::capacity() const noexcept
{
    return _data.capacity() * sizeof(UnsignedIntegerType) * 8;
}

template <typename UnsignedIntegerType>
void 
vector<UnsignedIntegerType>::shrink_to_fit() noexcept
{
    _data.shrink_to_fit(bsize / 8 + 1);
}

template <typename UnsignedIntegerType>
std::size_t 
vector<UnsignedIntegerType>::max_size() const noexcept
{
    return _data.max_size() * sizeof(UnsignedIntegerType) * 8;
}

template <typename UnsignedIntegerType>
void 
vector<UnsignedIntegerType>::swap(vector& other)
{
    _data.swap(other._data);
    std::swap(bsize, other.bsize);
}

template <typename UnsignedIntegerType>
std::size_t
vector<UnsignedIntegerType>::bit_to_byte_size(std::size_t bit_size) const noexcept 
{
    if (bit_size % block_bit_size) return bit_size / block_bit_size + 1;
    else return bit_size / block_bit_size;
}

template <typename UnsignedIntegerType>
std::tuple<std::size_t, std::size_t> 
vector<UnsignedIntegerType>::idx_to_coordinates(std::size_t idx) const noexcept
{
    std::size_t block_idx = idx / block_bit_size;
    std::size_t bit_idx = idx % block_bit_size;
    return std::make_tuple(block_idx, bit_idx);
}

template <typename UnsignedIntegerType>
void 
vector<UnsignedIntegerType>::check_coordinates(std::size_t idx) const
{
    if (idx >= bsize) throw std::out_of_range("[bit::vector] index to block coordinates");
}

template <typename UnsignedIntegerType>
template <class Visitor>
void 
vector<UnsignedIntegerType>::visit(Visitor& visitor)
{
    visitor.visit(_data);
    visitor.visit(bsize);
}

template <typename UnsignedIntegerType>
template <class Visitor>
void 
vector<UnsignedIntegerType>::visit(Visitor& visitor) const
{
    visitor.visit(_data);
    visitor.visit(bsize);
}

template <typename UnsignedIntegerType>
template <class Loader>
vector<UnsignedIntegerType> 
vector<UnsignedIntegerType>::load(Loader& visitor)
{
    vector<UnsignedIntegerType> r;
    r.visit(visitor);
    return r;
}

// ---------------------------------------------------------------------------------------------------------------

template <class UnsignedIntegerType>
vector<UnsignedIntegerType>::const_iterator::const_iterator(vector const& vec, std::size_t ref_idx)
    : parent_vector(vec), idx(ref_idx)
{
    if (idx > parent_vector.bsize) throw std::out_of_range("[bit::vector::const_iterator]");
    if (idx != parent_vector.bsize) {
        auto [block_idx, bit_idx] = parent_vector.idx_to_coordinates(idx);
        buffer = parent_vector._data.at(block_idx);
        buffer >>= bit_idx;
    }
}

template <class UnsignedIntegerType>
bool
vector<UnsignedIntegerType>::const_iterator::operator*() const
{
    assert(idx < parent_vector.bsize);
    return buffer & static_cast<UnsignedIntegerType>(1);
}

template <class UnsignedIntegerType>
typename vector<UnsignedIntegerType>::const_iterator const&
vector<UnsignedIntegerType>::const_iterator::operator++() noexcept
{
    auto [old_block_idx, old_bit_idx] = parent_vector.idx_to_coordinates(idx);
    ++idx;
    if (idx < parent_vector.size()) {
        auto [block_idx, bit_idx] = parent_vector.idx_to_coordinates(idx);
        if (old_block_idx != block_idx) {
            assert(block_idx - old_block_idx == 1);
            assert(bit_idx == 0);
            buffer = parent_vector._data.at(block_idx); // >> idx; useless, since idx is 0 here 
        } else {
            assert(bit_idx - old_bit_idx == 1);
            buffer >>= 1;
        }
    }
    return *this;
}

template <class UnsignedIntegerType>
typename vector<UnsignedIntegerType>::const_iterator
vector<UnsignedIntegerType>::const_iterator::operator++(int) noexcept
{
    auto current = *this;
    operator++();
    // auto [block_idx, bit_idx] = parent_vector.idx_to_coordinates(idx);
    // buffer = parent_vector._data.at(idx);
    return current;
}

template <class UnsignedIntegerType>
template <typename I>
typename vector<UnsignedIntegerType>::const_iterator
vector<UnsignedIntegerType>::const_iterator::operator+(I inc) const
{
    auto [block_idx, bit_idx] = parent_vector.idx_to_coordinates(idx + inc);
    auto toret = *this;
    toret.idx += inc;
    toret.buffer = toret.parent_vector._data.at(block_idx) >> bit_idx;
    return toret;
}

// -------------------------------------------------------------------------------------------------------

template <class UnsignedIntegerType>
vector<UnsignedIntegerType>::one_position_iterator::one_position_iterator(vector const& vec, std::size_t ref_idx)
    : parent_vector(vec), idx(ref_idx)
{
    if (idx >= parent_vector.size()) idx = parent_vector.size();
    find_next_one();
}

template <class UnsignedIntegerType>
std::size_t
vector<UnsignedIntegerType>::one_position_iterator::operator*() const
{
    return idx;
}

template <class UnsignedIntegerType>
typename vector<UnsignedIntegerType>::one_position_iterator const&
vector<UnsignedIntegerType>::one_position_iterator::operator++() noexcept
{
    ++idx;
    find_next_one();
    return *this;
}

template <class UnsignedIntegerType>
typename vector<UnsignedIntegerType>::one_position_iterator
vector<UnsignedIntegerType>::one_position_iterator::operator++(int) noexcept
{
    auto current = *this;
    operator++();
    return current;
}

template <class UnsignedIntegerType>
void
vector<UnsignedIntegerType>::one_position_iterator::find_next_one() noexcept
{
    while(idx < parent_vector.size() and not parent_vector.at(idx)) ++idx;
}

template <class UnsignedIntegerType>
template <typename I>
typename vector<UnsignedIntegerType>::one_position_iterator 
vector<UnsignedIntegerType>::one_position_iterator::operator+(I inc) noexcept
{
    auto toret = *this;
    toret.idx += inc;
    toret.find_next_one();
    return toret;
}

} // namespace bv

#endif // BIT_HPP