#include <iostream>
#include <numeric>
#include "../include/io.hpp"
#include "../include/logtools.hpp"
#include "../bundled/prettyprint.hpp"

class test_t
{
    public:
        template <class Visitor>
        void visit(Visitor& visitor) {
            visitor.visit(vec);
            visitor.visit(str);
            visitor.visit(value);
        }
        std::vector<uint32_t> vec;
        std::string str;
        int value;
};

int main()
{
    test_t source, sink;
    {
        for (std::size_t i = 0; i < 1000000; ++i) {
            source.vec.push_back(i);
        }
        source.str = "Hello, world!\n";
        source.value = 10;
        std::ofstream ofs("test_io_payload.bin", std::ios::binary);
        io::mut_saver saver(ofs);
        source.visit(saver);
        std::cerr << "Written " << saver.get_byte_size() << " Bytes\n";

        logging_tools::libra scale;
        source.visit(scale);
        std::cerr << "Weight " << scale.get_byte_size() << "\n";
    }

    {
        std::ifstream ifs("test_io_payload.bin", std::ios::binary);
        io::loader reader(ifs);
        sink.visit(reader);
        // std::cerr << lvec << "\n" << ls << "\n" << lval << "\n";
        std::cerr << "Read " << reader.get_byte_size() << " Bytes\n";

        logging_tools::libra scale;
        source.visit(scale);
        std::cerr << "Weight " << scale.get_byte_size() << "\n";
    }

    {
        bool equal_vec = source.vec == sink.vec;
        bool equal_s = source.str == sink.str;
        bool equal_val = source.value == sink.value;
        if (equal_vec and equal_s and equal_val) std::cerr << "PASS" << std::endl;
        else {
            std::cerr << "Failed" << std::endl;
            return 1; 
        }
    }
    return 0;
}