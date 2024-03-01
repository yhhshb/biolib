#include "../include/logtools.hpp"

namespace logging_tools {

libra::libra()
    : acc(0)
{}

void 
libra::visit(std::string const& s) noexcept
{
    acc += sizeof(decltype(s.size()));
    acc += s.size();
}

std::size_t 
libra::get_byte_size() const noexcept 
{
    return acc;
}

void
libra::reset() noexcept
{
    acc = 0;
}

} // namespace logging_tools