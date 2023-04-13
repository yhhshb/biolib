/**
 * K-mer view test
 */

#include <zlib.h>
extern "C" {
    #include "../include/kseq.h"
}

#include <string>
#include <argparse/argparse.hpp>
#include "../include/kmer_view.hpp"

KSEQ_INIT(gzFile, gzread)

typedef uint64_t kmer_t;

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
        wrapper::kmer_view<kmer_t> view(seq->seq.s, seq->seq.l, 15, true);
        for (auto itr = view.cbegin(); itr != view.cend(); ++itr) {
            if (*itr) dummy = **itr;
        }
    }
    if (seq) kseq_destroy(seq);
    gzclose(fp);

    std::cerr << "Finish\n";
    
    return 0;
}