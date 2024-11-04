#ifndef FEISTEL_HPP
#define FEISTEL_HPP

#include <vector>

namespace cryptography {
namespace cipher {

// class feistel
// {
//     public:
//         class digest
//         {
//             public:
//                 digest(uint8_t const * const unencrypted_data, std::size_t udlen);
//                 digest(std::vector<uint8_t> unencrypted_data);
//                 uint8_t const* data() const;
//                 std::size_t size() const;

//             private:
//                 void encrypt(uint8_t const * const unencrypted_data, std::size_t udlen);
//                 std::vector<uint8_t> _data;
//         };
//         feistel(std::vector<uint8_t> const& key, std::size_t rounds);
//         digest encrypt(uint8_t const * const unencrypted_data, std::size_t udlen) const;
//         std::vector<uint8_t> decrypt(digest const& encrypted_data) const;

//     private:
//         std::vector<uint8_t> _key;
//         std::size_t _rounds;
// };

} // namespace cypher
} // namespace cryptography

#endif // FEISTEL_HPP