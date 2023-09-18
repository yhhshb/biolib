#include <iostream>
#include "../include/io.hpp"
#include "../bundled/prettyprint.hpp"

int main()
{
    const std::vector<uint64_t> vec = {1,2,3,4,5,6,7,8};
    const std::string s = "Hello, world!\n";
    const int val = 10;
    {
        std::ofstream ofs("test_io_payload.bin", std::ios::binary);
        io::mut_saver saver(ofs);
        saver.visit(vec);
        saver.visit(s);
        saver.visit(val);
        std::cerr << "Written " << saver.get_byte_size() << " Bytes\n";
    }

    {
        std::vector<uint64_t> lvec;
        std::string ls;
        int lval;
        std::ifstream ifs("test_io_payload.bin", std::ios::binary);
        io::loader reader(ifs);
        reader.visit(lvec);
        reader.visit(ls);
        reader.visit(lval);
        // std::cerr << lvec << "\n" << ls << "\n" << lval << "\n";
        std::cerr << "Read " << reader.get_byte_size() << " Bytes\n";
        bool equal_vec = vec == lvec;
        bool equal_s = s == ls;
        bool equal_val = val == lval;
        if (equal_vec and equal_s and equal_val) std::cerr << "PASS" << std::endl;
        else {
            std::cerr << "Failed" << std::endl;
            return 1; 
        }
    }
    return 0;
}