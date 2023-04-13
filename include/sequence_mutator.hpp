#ifndef SEQUENCE_MUTATOR_HPP
#define SEQUENCE_MUTATOR_HPP

#include <random>
#include <optional>

#include "constants.hpp"

namespace wrapper {

template <class Iterator>
class sequence_mutator
{
    public:
        class const_iterator
        {
            public:
                struct report_t {
                    std::size_t substitution_count, insertion_count, deletion_count;
                    long long total_indel_len;
                    std::string cigar;
                };

                using iterator_category = std::forward_iterator_tag;
                using difference_type   = std::ptrdiff_t;
                using value_type        = std::optional<typename Iterator::value_type>;
                using pointer           = value_type*;
                using reference         = value_type&;

                enum class mutation_t {SUBSTITUTION, INSERTION, DELETION, IDENTITY};

                const_iterator(sequence_mutator const& mutator, Iterator const& start, uint64_t seed) noexcept;
                value_type operator*() const noexcept;
                const_iterator const& operator++() noexcept;
                const_iterator operator++(int) noexcept;
                report_t get_report() const noexcept;

            private:
                sequence_mutator const& parent_mutator;
                Iterator itr_start;
                std::mt19937 engine; // Standard mersenne_twister_engine seeded with rd()
                std::uniform_real_distribution<double> probability;
                mutation_t mtype;
                std::size_t indel_len;
                value_type outval;
                report_t report;

                friend bool operator==(const_iterator const& a, const_iterator const& b) 
                {
                    bool same_start = a.itr_start == b.itr_start;
                    return (a.parent_mutator == b.parent_mutator) and same_start;
                };
                friend bool operator!=(const_iterator const& a, const_iterator const& b) {return not (a == b);};
        };

        sequence_mutator(Iterator const& start, Iterator const& stop, double mutation_rate, double indel_fraction, double extension_probability);
        const_iterator cbegin(uint64_t seed) const noexcept;
        const_iterator cend() const noexcept;
        const_iterator begin(uint64_t seed) const noexcept {return cbegin(seed);};
        const_iterator end() const noexcept {return cend();};
        double get_mutation_probability() const noexcept {return mutp;};
        double get_indel_fraction() const noexcept {return indelf;};
        double get_extension_probability() const noexcept {return extp;};

        private:
            Iterator const itr_start;
            Iterator const itr_stop;
            double mutp, indelf, extp;

            friend bool operator==(sequence_mutator const& a, sequence_mutator const& b)
            {
                bool same_range = (a.itr_start == b.itr_start and a.itr_stop == b.itr_stop);
                bool same_mutp = (a.mutp == b.mutp);
                bool same_indelf = (a.indelf == b.indelf);
                bool same_extp = (a.extp == b.extp);
                return same_range and same_mutp and same_indelf and same_extp;
            };
            friend bool operator!=(sequence_mutator const& a, sequence_mutator const& b) {return not (a == b);};
};

template <typename Iterator>
sequence_mutator<Iterator>::sequence_mutator(Iterator const& start, Iterator const& stop, double mutation_rate, double indel_fraction, double extension_probability) 
    : itr_start(start), 
      itr_stop(stop), 
      mutp(mutation_rate), 
      indelf(indel_fraction), 
      extp(extension_probability) 
{
    if (mutp < 0 or mutp > 1) throw std::invalid_argument("[sequence_mutator] invalid mutation probability");
    if (indelf < 0 or indelf > 1) throw std::invalid_argument("[sequence_mutator] invalid indel fraction");
    if (extp < 0 or extp > 1) throw std::invalid_argument("[sequence_mutator] invalid extension probability");
}

template <class Iterator>
typename sequence_mutator<Iterator>::const_iterator 
sequence_mutator<Iterator>::cbegin(uint64_t seed) const noexcept
{
    return const_iterator(*this, itr_start, seed);
}

template <class Iterator>
typename sequence_mutator<Iterator>::const_iterator 
sequence_mutator<Iterator>::cend() const noexcept
{
    return const_iterator(*this, itr_stop, 0);
}

template <class Iterator>
sequence_mutator<Iterator>::const_iterator::const_iterator(sequence_mutator const& mutator, Iterator const& start, uint64_t seed) noexcept
    : parent_mutator(mutator), 
      itr_start(start), 
      mtype(mutation_t::IDENTITY), 
      indel_len(0),
      outval(std::nullopt),
      report({0, 0, 0, 0, ""})
{
    decltype(probability.param()) p(0.0, 1.0);
    probability.param(p);
    engine.seed(seed); // Standard mersenne_twister_engine seeded with rd()
    if (itr_start != parent_mutator.itr_stop) ++(*this);
}

template <class Iterator>
typename sequence_mutator<Iterator>::const_iterator::value_type 
sequence_mutator<Iterator>::const_iterator::operator*() const noexcept
{
    return outval;
}

template <class Iterator>
typename sequence_mutator<Iterator>::const_iterator const& 
sequence_mutator<Iterator>::const_iterator::operator++() noexcept
{
    if (indel_len == 0) {
        char base = *itr_start++;
        outval = base;
        uint8_t c = constants::seq_nt4_table.at(base);
        bool mutate = probability(engine) < parent_mutator.mutp;
        if (mutate) {
            if (probability(engine) >= parent_mutator.indelf) { // substitution
                mtype = mutation_t::SUBSTITUTION;
                c += static_cast<uint8_t>(probability(engine) * 3 + 1);
                c &= 3;
                ++report.substitution_count;
                outval = constants::bases.at(c);
            } else { // indel
                indel_len = 1;
                while (probability(engine) < parent_mutator.extp) ++indel_len; // series of Bernoulli trials
                if (probability(engine) < 0.5) {
                    mtype = mutation_t::DELETION;
                    ++report.deletion_count;
                    report.total_indel_len -= indel_len;
                    outval = std::nullopt;
                } else {
                    mtype = mutation_t::INSERTION;
                    ++report.insertion_count;
                    report.total_indel_len += indel_len;
                    assert(constants::bases.at(c) == base);
                    outval = constants::bases.at(c);
                }
            }
        } // else {mtype = mutation_t::IDENTITY}
    } else {
        if (mtype == mutation_t::INSERTION or mtype == mutation_t::DELETION) --indel_len;
        if (indel_len == 0) mtype = mutation_t::IDENTITY;
    }
    return *this;
}

template <class Iterator>
typename sequence_mutator<Iterator>::const_iterator 
sequence_mutator<Iterator>::const_iterator::operator++(int) noexcept
{
    auto current = *this;
    operator++();
    return current;
}

template <class Iterator>
typename sequence_mutator<Iterator>::const_iterator::report_t 
sequence_mutator<Iterator>::const_iterator::get_report() const noexcept
{
    return report;
}

} // namespace wrapper

#endif // SEQUENCE_MUTATOR_HPP