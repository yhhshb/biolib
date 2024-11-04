#include "../include/feistel.hpp"

namespace cryptography::cipher {

// feistel::digest::digest(uint8_t const * const unenrypted_data, std::size_t udlen)
// {
//     encrypt(unenrypted_data, udlen);
// }

// feistel::digest::digest(std::vector<uint8_t> unencrypted_data)
// {
//     encrypt(unencrypted_data.data(), unencrypted_data.size());
// }

// uint8_t const*
// feistel::digest::data() const
// {
//     return _data.data();
// }

// std::size_t
// feistel::digest::size() const
// {
//     return _data.size();
// }

// void 
// feistel::digest::encrypt(uint8_t const * const unencrypted_data, std::size_t udlen)
// {
//     //TODO
// }

// feistel::feistel(std::vector<uint8_t> const& key, std::size_t rounds)
// {
//     _key = key;
//     _rounds = rounds;
// }

// feistel::digest
// feistel::encrypt(uint8_t const * const unencrypted_data, std::size_t udlen) const
// {
//     return digest(unencrypted_data, udlen);
// }

// std::vector<uint8_t>
// feistel::decrypt(digest const& encrypted_data) const
// {
//     std::vector<uint8_t> res;
//     for (std::size_t i = 0; i < _rounds; ++i) {
//         // perform one encryption round
//     }
//     return res;
// }

}