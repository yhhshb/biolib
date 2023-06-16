#ifndef LOGTOOLS_HPP
#define LOGTOOLS_HPP

#include <cstddef>
#include <string>

namespace logging_tools {

template <typename T>
static std::size_t basic_size_measure([[maybe_unused]] T const& val)
{
    static_assert(std::is_fundamental<T>::value);
    if constexpr (std::is_array<T>::value) throw std::domain_error("[Function load] C arrays are not supported");
    return sizeof(T);
}

template <typename T, typename Allocator>
static std::size_t basic_size_measure(std::vector<T, Allocator> const& vec)
{
    auto tmp = vec.size();
    std::size_t bytes_read = sizeof(decltype(tmp));
    for (auto& v : vec) bytes_read += basic_size_measure(v);
    return bytes_read;
}

class libra
{
    public:
        libra() : acc(0) {}

        template <typename T>
        void apply(T const& var) noexcept;

        template <typename T, class Allocator>
        void apply(std::vector<T, Allocator> const& vec) noexcept;

        std::size_t get_byte_size() const noexcept {return acc;}

    private:
        std::size_t acc;
};

template <typename T>
void libra::apply(T const& var) noexcept
{
    if constexpr (std::is_fundamental<T>::value) {
        auto nr = basic_size_measure(var);
        acc += nr;
    } else {
        var.visit(*this); // supposing the to-be measured object has a visit() method
    }
}

template <typename T, typename Allocator>
void libra::apply(std::vector<T, Allocator> const& vec) noexcept
{
    if constexpr (std::is_fundamental<T>::value) {
        auto nr = basic_size_measure(vec);
        acc += nr;
    } else {
        auto n = vec.size();
        apply(n);
        for (auto const& v : vec) apply(v); // Call apply(), not load() since we want to recursively count the number of bytes
    }
} 

} // namespace logging_tools

#endif