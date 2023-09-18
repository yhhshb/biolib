#include <iostream>
#include "../include/codes.hpp"
#include "../bundled/prettyprint.hpp"

int main()
{
    {
        bit::vector<uint8_t> vec;
        std::vector<uint8_t> result = {32, 136, 65, 138, 57, 40, 1};
        for (std::size_t i = 0; i < 10; ++i) {
            vec.push_back(static_cast<uint8_t>(i), 5);
        }
        // for (auto v : vec.vector_data()) std::cerr << std::size_t(v) << ",";
        bool check_push_back = (vec.vector_data() == result);
        if (not check_push_back) {
            std::cerr << "FAIL append check\n";
            return 1;
        }
    }
    {
        bit::vector<uint64_t> vec;
        std::vector<uint64_t> result;
        for (uint64_t i = 0; i < 10; ++i) {
            vec.push_back(i, 64);
            result.push_back(i);
        }
        bool check_push_back = (vec.vector_data() == result);
        if (not check_push_back) {
            std::cerr << "FAIL block len appends\n";
            return 1;
        }
    }
    {
        bit::vector<uint64_t> vec;
        bit::encoder::delta(vec, 5);
        std::cerr << "PASS\n";
    }
    return 0;
}