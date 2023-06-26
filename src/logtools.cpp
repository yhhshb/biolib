#include "../include/logtools.hpp"

namespace logging_tools {

libra::libra()
    : acc(0)
{}

std::size_t 
libra::get_byte_size() const noexcept 
{
    return acc;
}

} // namespace logging_tools