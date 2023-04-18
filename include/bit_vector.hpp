#ifndef BIT_HPP
#define BIT_HPP

#include <vector>

namespace bit {

template <typename UnsignedIntegerType>
class vector
{
    public:
        using value_type = UnsignedIntegerType;
        using block_type = UnsignedIntegerType;

        vector() noexcept;
        vector(std::size_t size, bool val = false);
        void reserve(std::size_t capacity);
        void resize(std::size_t size);
        void resize(std::size_t size, bool value);
        void clear();

        void push_back(bool bit);
        void push_back(UnsignedIntegerType block, std::size_t suffix_len);

        void set(std::size_t idx);
        void clear(std::size_t idx);
        bool at(std::size_t idx) const;

        UnsignedIntegerType& operator[](std::size_t idx);
        UnsignedIntegerType const& block_at(std::size_t idx) const;

        UnsignedIntegerType const* data() const noexcept;
        std::vector<UnsignedIntegerType> const& data(int) const noexcept;
        std::size_t block_size() const noexcept;
        std::size_t size() const noexcept;
        void swap(vector& other);

        template <class Visitor>
        void visit(Visitor& visitor);
        
    private:
        const std::size_t block_bit_size;
        std::size_t bsize;
        std::vector<UnsignedIntegerType> _data;

        std::size_t bit_to_byte_size(std::size_t bit_size) const noexcept;
        std::tuple<std::size_t, std::size_t> idx_to_coordinates(std::size_t idx) const;
};

template <typename UnsignedIntegerType>
vector<UnsignedIntegerType>::vector() noexcept 
    : block_bit_size(sizeof(UnsignedIntegerType) * 8), bsize(0)
{}

template <typename UnsignedIntegerType>
vector<UnsignedIntegerType>::vector(std::size_t size, bool val) 
    : block_bit_size(sizeof(UnsignedIntegerType) * 8), bsize(size)
{
    _data.resize(block_size(), static_cast<UnsignedIntegerType>(val));
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
    if (a == data.size()) _data.push_back(static_cast<block_type>(0));
    ++bsize;
    auto [block_idx, bit_idx] = idx_to_coordinates(bsize);
    _data[block_idx] |= static_cast<block_type>(bit) << (bit_idx);
}

template <typename UnsignedIntegerType>
void 
vector<UnsignedIntegerType>::push_back(UnsignedIntegerType block, std::size_t suffix_len)
{
    auto [a, b] = idx_to_coordinates(bsize + suffix_len);
    if (a >= data.size()) {
        _data.push_back(static_cast<block_type>(block >> (block_bit_size - b)));
    }
    bsize += suffix_len;
    auto [block_idx, bit_idx] = idx_to_coordinates(bsize);
    data[block_idx] |= block << b;
}

template <typename UnsignedIntegerType>
void 
vector<UnsignedIntegerType>::set(std::size_t idx)
{
    auto [block_idx, bit_idx] = idx_to_coordinates(idx);
    data[block_idx] |= static_cast<UnsignedIntegerType>(1) << bit_idx;
}

template <typename UnsignedIntegerType>
void 
vector<UnsignedIntegerType>::clear(std::size_t idx)
{
    auto [block_idx, bit_idx] = idx_to_coordinates(idx);
    data[block_idx] &= ~(static_cast<UnsignedIntegerType>(1) << bit_idx);
}

template <typename UnsignedIntegerType>
bool 
vector<UnsignedIntegerType>::at(std::size_t idx) const
{
    auto [block_idx, bit_idx] = idx_to_coordinates(idx);
    return static_cast<bool>(data.at(block_idx) & (static_cast<UnsignedIntegerType>(1) << bit_idx));
}

template <typename UnsignedIntegerType>
UnsignedIntegerType&
vector<UnsignedIntegerType>::operator[](std::size_t idx)
{
    auto [block_idx, [[maybe_unused]] bit_idx] = idx_to_coordinates(idx);
    return data[block_idx];
}

template <typename UnsignedIntegerType>
UnsignedIntegerType const&
vector<UnsignedIntegerType>::block_at(std::size_t idx) const
{
    auto [block_idx, [[maybe_unused]] bit_idx] = idx_to_coordinates(idx);
    return data.at(block_idx);
}

template <typename UnsignedIntegerType>
UnsignedIntegerType const* 
vector<UnsignedIntegerType>::data() const noexcept
{
    return _data.data();
}

template <typename UnsignedIntegerType>
std::vector<UnsignedIntegerType> const& 
vector<UnsignedIntegerType>::data(int) const noexcept
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
void 
vector<UnsignedIntegerType>::swap(vector& other)
{
    _data.swap(other._data);
    // std::swap(block_bit_size, other.block_bit_size);
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
    if (idx >= bsize) throw std::out_of_range("[bit vector set]");
    std::size_t block_idx = idx / block_bit_size;
    std::size_t bit_idx = idx % block_bit_size;
    return std::make_tuple(block_idx, bit_idx);
}

template <typename UnsignedIntegerType>
template <class Visitor>
void vector<UnsignedIntegerType>::visit(Visitor& visitor)
{
    visitor.apply(bsize);
    visitor.apply(_data);
}

} // namespace bv

#endif // BIT_HPP