#ifndef TOOLBOX_HPP
#define TOOLBOX_HPP

#include <cinttypes>
#include <type_traits>

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

} // namespace toolbox

#endif // TOOLBOX_HPP