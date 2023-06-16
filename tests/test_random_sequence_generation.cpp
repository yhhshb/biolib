/*
Random sequence generation and mutation
*/

#include <argparse/argparse.hpp>
#include "../include/sequence_generator.hpp"
#include "../include/sequence_mutator.hpp"

int main(int argc, char* argv[])
{
    argparse::ArgumentParser parser(argv[0]);
    parser.add_argument("length")
        .help("sequence length")
        .scan<'d', uint64_t>()
        .required();
    parser.add_argument("-m", "--mutation-rate")
        .help("mutation rate")
        .scan<'g', double>()
        .required();
    parser.add_argument("-d", "--indel-fraction")
        .help("fraction of mutations that are indels")
        .scan<'g', double>()
        .default_value(double(0));
    parser.add_argument("-e", "--extension-probability")
        .help("probability of extending an indel to more than one base (lengths are Bernoulli distributed)")
        .scan<'g', double>()
        .default_value(double(0.0));
    parser.add_argument("-s", "--seed")
        .help("random seed")
        .scan<'d', uint64_t>()
        .default_value(uint64_t(42));
    
    parser.parse_args(argc, argv);
    std::size_t seqlen = static_cast<std::size_t>(parser.get<uint64_t>("length"));
    auto mrate = parser.get<double>("--mutation-rate");
    auto indel = parser.get<double>("--indel-fraction");
    auto eprob = parser.get<double>("--extension-probability");
    auto seed =  parser.get<uint64_t>("--seed");

    sequence::generator::packed::uniform nucleic2bitgen(seed);
    sequence::generator::nucleic generator(nucleic2bitgen);
    auto original = generator.get_sequence(seqlen);
    wrapper::sequence_mutator mutated_view(original.begin(), original.end(), mrate, indel, eprob);
    std::string mutated;
    decltype(mutated_view)::const_iterator::report_t stats;
    for (auto itr = mutated_view.cbegin(seed); itr != mutated_view.cend(); ++itr) {
        if (*itr) mutated.push_back(**itr);
        stats = itr.get_report();
    }
    std::cout << original << "\n";
    std::cout << mutated << "\n";

    std::cerr << "substitutions: " << stats.substitution_count << "\n";
    std::cerr << "insertions events: " << stats.insertion_count << "\n";
    std::cerr << "deletions events: " << stats.deletion_count << "\n";
    std::cerr << "mutation events: " << stats.substitution_count + stats.insertion_count + stats.deletion_count << "\n";
    std::cerr << "total indel length: " << stats.total_indel_len << "\n";
}