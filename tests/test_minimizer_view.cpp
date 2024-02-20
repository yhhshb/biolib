/**
 * minimizer view test
 */

#include <zlib.h>
extern "C" {
    #include "kseq.h"
}

#include <string>
#include <argparse/argparse.hpp>
#include "../include/hash.hpp"
#include "../include/minimizer_view.hpp"

KSEQ_INIT(gzFile, gzread)

typedef uint64_t kmer_t;
typedef uint64_t mmer_t;

int main(int argc, char* argv[])
{
    gzFile fp;
    kseq_t* seq;

    argparse::ArgumentParser parser(argv[0]);
    parser.add_argument("-i", "--input")
        .help("input fasta file")
        .required();
    parser.parse_args(argc, argv);
    std::string input_filename = parser.get<std::string>("--input");

    if ((fp = gzopen(input_filename.c_str(), "r")) == NULL) throw std::runtime_error("Unable to open the input file " + input_filename + "\n");
    seq = kseq_init(fp);

    volatile kmer_t dummy;

    while (kseq_read(seq) >= 0) {
        auto view = wrapper::minimizer_view_from_cstr<kmer_t, mmer_t, hash::hash64>(seq->seq.s, seq->seq.l, (uint8_t)15, (uint8_t)10, 42, false);
        for (auto itr = view.cbegin(); itr != view.cend(); ++itr) {
            auto val = *itr;
            std::cerr << val.value << " | " << val.position << " | " << val.id << std::endl;
        }
    }
    if (seq) kseq_destroy(seq);
    gzclose(fp);

    std::cerr << "Finish\n";
    
    return 0;
}