#ifndef SEQUENCE_GENERATOR_HPP
#define SEQUENCE_GENERATOR_HPP

#include <string>
#include <random>

#include "constants.hpp"

namespace sequence {
namespace generator {

namespace packed {

class uniform
{
    public:
        uniform(uint64_t seed) noexcept : engine(seed), dist(0,3) {}
        uint8_t operator()() noexcept {return dist(engine);}

    private:
        std::mt19937 engine; // Standard mersenne_twister_engine seeded with rd()
        std::uniform_int_distribution<uint8_t> dist;
};

} // namespace packed

template <class Packed>
class nucleic
{
    public:
        nucleic(Packed& generator) noexcept : mgenerator(generator) {}
        char get_char() noexcept {return constants::bases.at(mgenerator());}

        std::string get_sequence(std::size_t len) noexcept
        {
            std::string output(len, 'N');
            for (std::size_t i = 0; i < len; ++i) output[i] = get_char();
            return output;
        }

    private:
        Packed& mgenerator;
};

} // namespace generator
} // namespace sequence

#endif // SEQUENCE_GENERATOR_HPP