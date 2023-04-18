
#include <cassert>
#include <argparse/argparse.hpp>
#include "../include/bit_vector.hpp"

typedef uint64_t uit;

int main() //int argc, char* argv[])
{
    // argparse::ArgumentParser parser(argv[0]);
    // parser.add_argument("-d", "--tmp-dir")
    //     .help("Temporary directory where to save vector chunks")
    //     .required();
    // parser.parse_args(argc, argv);
    // std::string tmp_dir = parser.get<std::string>("--tmp-dir");

    bit::vector<uit> bvec;
    assert(bvec.size() == 0);
    bvec.resize(1000, false);
    auto vec = bvec.data(0);
    for (auto itr = vec.cbegin(); itr != vec.cend(); ++itr) {
        assert(*itr == static_cast<uit>(0));
    }
    
    auto vec = bvec.data();
    for (std::size_t i = 0; i < bvec.block_size(); ++i) {
        assert(bvec.block_at(i) == static_cast<uit>(0));
    }

    return 0;
}