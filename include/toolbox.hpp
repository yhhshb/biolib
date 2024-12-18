#ifndef TOOLBOX_HPP
#define TOOLBOX_HPP

#include <cinttypes>
#include <type_traits>
#include <string_view>

namespace toolbox {

struct null_t {};

template <typename T>
class precision_selector {public: using type = null_t;}; // implement this one as well, if you want to have a default...

#if defined(__SIZEOF_INT128__)
template <> class precision_selector<uint64_t> {public: using type = __uint128_t;};
#else
template <> class precision_selector<uint64_t> {public: using type = null_t;};
#endif
template <> class precision_selector<uint32_t> {public: using type = uint64_t;};
template <> class precision_selector<uint16_t> {public: using type = uint32_t;};
template <> class precision_selector<uint8_t>  {public: using type = uint16_t;};

template <typename I>
static inline I fastrange(I word, uint8_t p)
{
	using B = typename precision_selector<I>::type;
	if constexpr (std::is_same<B, null_t>::value) return word % static_cast<I>(p);
	else return static_cast<I>((static_cast<B>(word) * static_cast<B>(p)) >> (8 * sizeof(I)));
}

template <class T>
constexpr
std::string_view
type_name()
{
    using namespace std;
#ifdef __clang__
    string_view p = __PRETTY_FUNCTION__;
    return string_view(p.data() + 34, p.size() - 34 - 1);
#elif defined(__GNUC__)
    string_view p = __PRETTY_FUNCTION__;
#  if __cplusplus < 201402
    return string_view(p.data() + 36, p.size() - 36 - 1);
#  else
    return string_view(p.data() + 49, p.find(';', 49) - 49);
#  endif
#elif defined(_MSC_VER)
    string_view p = __FUNCSIG__;
    return string_view(p.data() + 84, p.size() - 84 - 7);
#endif
}

template <typename T>
void print_bits(T v) {
    static_assert(std::is_unsigned<T>::value);
    for(std::size_t i = 8 * sizeof(T) - 1; i != std::numeric_limits<std::size_t>::max(); --i) putc('0' + ((v >> i) & 1), stderr);
}

template <typename T>
void print_bits_reverse(T v) {
    static_assert(std::is_unsigned<T>::value);
    for(std::size_t i = 0; i < 8 * sizeof(T); ++i) putc('0' + ((v >> i) & 1), stderr);
}

} // namespace toolbox

#endif // TOOLBOX_HPP