/**
 * External (Sorted) Memory Vector test
 */

#include <random>
#include <string>
#include <argparse/argparse.hpp>
#include "../include/external_memory_vector.hpp"
#include "../bundled/prettyprint.hpp"

int main(int argc, char* argv[])
{
    argparse::ArgumentParser parser(argv[0]);
    parser.add_argument("-d", "--tmp-dir")
        .help("Temporary directory where to save vector chunks")
        .required();
    parser.parse_args(argc, argv);
    std::string tmp_dir = parser.get<std::string>("--tmp-dir");

    const uint64_t n = 10000;

    { // sorted vector: vector memory < input elements
        emem::external_memory_vector<uint64_t> sorted_vec(1000, tmp_dir, "kmp_test_sorted_emv");
        for (uint64_t i = n-1; i < std::numeric_limits<uint64_t>::max(); --i) {
            sorted_vec.push_back(i);
        }
        sorted_vec.minimize();
        auto itr = sorted_vec.cbegin();
        for (uint64_t i = 0; i < n; ++i) {
            auto val = *itr;
            if (val != i) {
                std::cerr << "FAILURE : sorted vector (without enough memory)" << std::endl;
                return 1;
            }
            ++itr;
        }

        std::cerr << "PASS : sorted vector (without enough memory)\n";
    }

    { // sorted vector: vector memory > input elements
        emem::external_memory_vector<uint64_t> sorted_vec(100000, tmp_dir, "kmp_test_sorted_emv");
        for (uint64_t i = n-1; i < std::numeric_limits<uint64_t>::max(); --i) {
            sorted_vec.push_back(i);
        }
        sorted_vec.minimize();
        auto itr = sorted_vec.cbegin();
        for (uint64_t i = 0; i < n; ++i) {
            auto val = *itr;
            if (val != i) {
                std::cerr << "FAILURE : sorted vector (with enough memory)" << std::endl;
                return 1;
            }
            ++itr;
        }

        std::cerr << "PASS : sorted vector (with enough memory)\n";
    }

    { // sorted vector: complex elements
        std::size_t size = 10;
        std::mt19937 gen(42); // Standard mersenne_twister_engine seeded with rd()
        emem::external_memory_vector<std::pair<std::vector<uint32_t>, uint64_t>> sorted_vec(10000, tmp_dir, "kmp_test_sorted_emv");
        {
            std::uniform_int_distribution<std::size_t> lengths(0, 10);
            std::uniform_int_distribution<uint32_t> values(0, 100);
            for (uint64_t i = 0; i < size; ++i) {
                std::vector<uint32_t> key;
                for (std::size_t j = 0; j < lengths(gen); ++j) {
                    key.push_back(values(gen));
                }
                auto pair = std::make_pair(key, i);
                std::cerr << pair << "\n";
                sorted_vec.push_back(pair);
            }
        }
        // sorted_vec.minimize();
        std::cerr << "\nSorted pairs:\n" ;
        for (auto itr = sorted_vec.cbegin(); itr != sorted_vec.cend(); ++itr) {
            std::cerr << *itr << "\n";
        }
    }

    { // unsorted vector
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