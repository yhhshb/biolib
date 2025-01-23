#ifndef SEQUENCE_GENERATOR_HPP
#define SEQUENCE_GENERATOR_HPP

#include <string>
#include <random>

#include "constants.hpp"

namespace DNA {
namespace sequence {

class generator
{
    public:
        generator(uint64_t seed) noexcept : engine(seed), dist(0,3) {}
        char get_char() noexcept {return constants::bases.at(dist(engine));}

        std::string get_sequence(std::size_t len) noexcept
        {
            std::string output(len, 'N');
            for (std::size_t i = 0; i < len; ++i) output[i] = get_char();
            return output;
        }

    private:
        std::mt19937 engine; // Standard mersenne_twister_engine seeded with rd()
        std::uniform_int_distribution<uint8_t> dist;
};

} // namespace sequence
} // namespace DNA

#endif // SEQUENCE_GENERATOR_HPP