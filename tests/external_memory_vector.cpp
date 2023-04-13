/**
 * External (Sorted) Memory Vector test
 */

#include <string>
#include <argparse/argparse.hpp>
#include "../include/external_memory_vector.hpp"

int main(int argc, char* argv[])
{
    argparse::ArgumentParser parser(argv[0]);
    parser.add_argument("-d", "--tmp-dir")
        .help("Temporary directory where to save vector chunks")
        .required();
    parser.parse_args(argc, argv);
    std::string tmp_dir = parser.get<std::string>("--tmp-dir");

    const uint64_t n = 10000;

    {
        emem::external_memory_vector<uint64_t> sorted_vec(1000, tmp_dir, "kmp_test_sorted_emv");
        for (uint64_t i = n-1; i < std::numeric_limits<uint64_t>::max(); --i) {
            sorted_vec.push_back(i);
        }

        auto itr = sorted_vec.cbegin();
        for (uint64_t i = 0; i < n; ++i) {
            auto val = *itr;
            if (val != i) {
                std::cerr << "FAILURE : sorted vector" << std::endl;
                return 1;
            }
            ++itr;
        }

        std::cerr << "PASS : sorted vector\n";
    }

    {
        emem::external_memory_vector<uint64_t, false> unsorted_vec(1000, tmp_dir, "kmp_test_unsorted_emv");
        for (uint64_t i = n-1; i < std::numeric_limits<uint64_t>::max(); --i) {
            unsorted_vec.push_back(i);
        }

        auto itr = unsorted_vec.cbegin();
        for (uint64_t i = n-1; i < std::numeric_limits<uint64_t>::max(); --i) {
            auto val = *itr;
            if (val != i) {
                std::cerr << "FAILURE : unsorted vector" << std::endl;
                return 1;
            }
            ++itr;
        }

        std::cerr << "PASS : unsorted vector\n";
    }

    {
        std::vector<uint64_t> bla;
        bla.reserve(n);
        for (std::size_t i = 0; i < n; ++i) bla.push_back(i);
        emem::external_memory_vector<uint64_t> v(1000, tmp_dir, "kmp_test_emv_back_inserter"); 
        std::set_union(bla.begin(), bla.end(), bla.begin(), bla.end(), std::back_inserter(v));
        std::cerr << "PASS : set union done\n";
    }
    
    return 0;
}