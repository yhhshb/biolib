/* 
 * super-k-mer view test and comparison to C version
 */

#include <zlib.h>
extern "C" {
    #include "kseq.h"
}

#include <string>
#include <argparse/argparse.hpp>
#include "../include/super_kmer_view.hpp"
#include "../include/hash.hpp"

KSEQ_INIT(gzFile, gzread)

typedef __uint128_t kmer_t;
typedef uint64_t mm_t;

int main(int argc, char *argv[]) 
{
    gzFile fp;
    kseq_t* seq;
    std::string input_filename;
    uint8_t k, m;

    if ((fp = gzopen(input_filename.c_str(), "r")) == NULL) throw std::runtime_error("Unable to open the input file " + input_filename + "\n");
    seq = kseq_init(fp);
    while (kseq_read(seq) >= 0) {
        wrapper::super_kmer_view<kmer_t, mm_t, hash::hash64> view(seq->seq.s, seq->seq.l, k, m, false);
        for (auto itr = view.cbegin(); itr != view.cend(); ++itr) {
            // TODO
        }
    }
    if (seq) kseq_destroy(seq);
    gzclose(fp);
}