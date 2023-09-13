/*
 Compute Jaccard between two sequences
*/

#include <string>
#include <argparse/argparse.hpp>
#include "../include/external_memory_vector.hpp"
#include "../include/kmer_view.hpp"
#include "../include/ordered_unique_sampler.hpp"

#include <zlib.h>
extern "C" {
    #include "kseq.h"
}

KSEQ_INIT(gzFile, gzread)

typedef uint64_t kmer_t;

int main(int argc, char* argv[])
{
    gzFile fp;
    kseq_t* seq;

    argparse::ArgumentParser parser(argv[0]);
    parser.add_argument("first")
        .required();
    parser.add_argument("second")
        .required();
    parser.add_argument("-k")
        .help("k-mer length")
        .scan<'u', uint16_t>()
        .required();
    parser.add_argument("-c", "--canonical")
        .help("Canonical k-mers")
        .implicit_value(true)
        .default_value(false);
    parser.add_argument("-d", "--tmp-dir")
        .help("Temporary directory where to save vector chunks")
        .required();
    parser.add_argument("-m", "--max-ram")
        .help("RAM limit in MB")
        .scan<'d', uint64_t>()
        .default_value(uint64_t(200));
    parser.parse_args(argc, argv);
    std::string first_fasta = parser.get<std::string>("first");
    std::string second_fasta = parser.get<std::string>("second");
    uint16_t k = parser.get<uint16_t>("-k");
    bool canonical = parser.get<bool>("--canonical");
    std::string tmp_dir = parser.get<std::string>("--tmp-dir");
    auto max_ram_bytes = parser.get<uint64_t>("--max-ram") * 1000000;

    std::cerr << max_ram_bytes << std::endl;
    
    emem::external_memory_vector<kmer_t> kmer_vector_first(max_ram_bytes, tmp_dir, "first");
    emem::external_memory_vector<kmer_t> kmer_vector_second(max_ram_bytes, tmp_dir, "second");

    if ((fp = gzopen(first_fasta.c_str(), "r")) == NULL) throw std::runtime_error("Unable to open the input file " + first_fasta + "\n");
    seq = kseq_init(fp);
    while (kseq_read(seq) >= 0) {
        auto view = wrapper::kmer_view_from_cstr<kmer_t>(seq->seq.s, seq->seq.l, k, canonical);
        for (auto itr = view.cbegin(); itr != view.cend(); ++itr) {
            if ((*itr).value) kmer_vector_first.push_back(*((*itr).value));
        }
    }
    if (seq) kseq_destroy(seq);
    gzclose(fp);

    if ((fp = gzopen(second_fasta.c_str(), "r")) == NULL) throw std::runtime_error("Unable to open the input file " + second_fasta + "\n");
    seq = kseq_init(fp);
    while (kseq_read(seq) >= 0) {
        auto view = wrapper::kmer_view_from_cstr<kmer_t>(seq->seq.s, seq->seq.l, k, canonical);
        for (auto itr = view.cbegin(); itr != view.cend(); ++itr) {
            if ((*itr).value) kmer_vector_second.push_back(*((*itr).value));
        }
    }
    if (seq) kseq_destroy(seq);
    gzclose(fp);

    std::cerr << "found " << kmer_vector_first.size() << " and " << kmer_vector_second.size() << " k-mers" << std::endl;
    std::size_t j = 0;
    for (auto it = kmer_vector_first.cbegin(); it != kmer_vector_first.cend() and j < 100; ++j, ++it) {
        std::cerr << *it << "\n";
    }
    j = 0;
    std::cerr << "----------------------------------------------------------------\n";
    for (auto it = kmer_vector_second.cbegin(); it != kmer_vector_second.cend() and j < 100; ++j, ++it) {
        std::cerr << *it << "\n";
    }
    std::cerr << std::endl;

    sampler::ordered_unique_sampler unique_kmers_first(kmer_vector_first.cbegin(), kmer_vector_first.cend());
    sampler::ordered_unique_sampler unique_kmers_second(kmer_vector_second.cbegin(), kmer_vector_second.cend());
    std::size_t unique_kmers_size_first = 0;
    std::size_t unique_kmers_size_second = 0;
    auto itr_first = unique_kmers_first.cbegin();
    auto itr_second = unique_kmers_second.cbegin();
    auto first_ok = [&unique_kmers_first, &itr_first]() {
        return itr_first != unique_kmers_first.cend();
    };
    auto second_ok = [&unique_kmers_first, &itr_second]() {
        return itr_second != unique_kmers_first.cend();
    };
    std::size_t unione, intersection;
    unione = intersection = 0;
    while(first_ok() or second_ok())
	{
		if(first_ok() and second_ok() and *itr_first == *itr_second)
		{
			unione += 1;
			intersection += 1;
            ++itr_first;
            ++itr_second;
            ++unique_kmers_size_first;
            ++unique_kmers_size_second;
		}
		else if (first_ok() and (not second_ok() or *itr_first < *itr_second))
		{
			unione += 1;
			++itr_first;
            ++unique_kmers_size_first;
		}
		else if (second_ok() and (not first_ok() or *itr_second < *itr_first))
		{
			unione += 1;
            ++itr_second;
            ++unique_kmers_size_second;
		}
	}
    std::cout << "Jaccard : " << intersection << "/" << unione << " = " << double(intersection) / unione << "\n";
}