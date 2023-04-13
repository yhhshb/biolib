#ifndef SYNCMER_SAMPLER_HPP
#define SYNCMER_SAMPLER_HPP

#include <vector>
#include <cassert>

namespace sampler {

template <class Iterator, typename PropertyExtractor>
class syncmer_sampler
{
    public:
        class const_iterator
        {
            public:
                using iterator_category = std::forward_iterator_tag;
                using difference_type   = std::ptrdiff_t;
                using value_type        = typename PropertyExtractor::value_type;
                using pointer           = value_type*;
                using reference         = value_type&;
                
                const_iterator(syncmer_sampler const& sampler, Iterator const& start);
                value_type operator*() const;
                const_iterator const& operator++();
                const_iterator operator++(int);

            private:
                syncmer_sampler const& parent_sampler;
                Iterator itr_start;

                void find_first_syncmer() noexcept;

                friend bool operator==(const_iterator const& a, const_iterator const& b) 
                {
                    bool same_start = a.itr_start == b.itr_start;
                    return (a.parent_sampler == b.parent_sampler) and same_start;
                };
                friend bool operator!=(const_iterator const& a, const_iterator const& b) {return not (a == b);};

                template <typename T>
                T optional_unwrap(T const& val) const noexcept {return val;}

                template <typename T>
                T optional_unwrap(std::optional<T> const& opt) const noexcept {return *opt;}
        };

        syncmer_sampler(Iterator const& start, Iterator const& stop, PropertyExtractor const& extractor, uint16_t start_offset, uint16_t end_offset);
        const_iterator cbegin() const;
        const_iterator cend() const;
        const_iterator begin() const {return cbegin();};
        const_iterator end() const {return cend();};
        std::pair<uint16_t, uint16_t> get_offsets() const;

    private:
        Iterator const itr_start;
        Iterator const itr_stop;
        PropertyExtractor const& extor;
        uint16_t soffset;
        uint16_t eoffset;

        friend bool operator==(syncmer_sampler const& a, syncmer_sampler const& b)
        {
            bool same_range = (a.itr_start == b.itr_start and a.itr_stop == b.itr_stop);
            return same_range;
        };
        friend bool operator!=(syncmer_sampler const& a, syncmer_sampler const& b) {return not (a == b);};
};

template <class Iterator, typename PropertyExtractor>
syncmer_sampler<Iterator, PropertyExtractor>::syncmer_sampler(Iterator const& start, Iterator const& stop, PropertyExtractor const& extractor, uint16_t start_offset, uint16_t end_offset) 
    : itr_start(start), itr_stop(stop), extor(extractor), soffset(start_offset), eoffset(end_offset)
{}

template <class Iterator, typename PropertyExtractor>
typename syncmer_sampler<Iterator, PropertyExtractor>::const_iterator syncmer_sampler<Iterator, PropertyExtractor>::cbegin() const
{
    return const_iterator(*this, itr_start);
}

template <class Iterator, typename PropertyExtractor>
typename syncmer_sampler<Iterator, PropertyExtractor>::const_iterator syncmer_sampler<Iterator, PropertyExtractor>::cend() const
{
    return const_iterator(*this, itr_stop);
}

template <class Iterator, typename PropertyExtractor>
std::pair<uint16_t, uint16_t> syncmer_sampler<Iterator, PropertyExtractor>::get_offsets() const
{
    return std::make_pair(soffset, eoffset);
}

//--------------------------------------------------------------------------------------------------------------------------------------

template <class Iterator, typename PropertyExtractor>
syncmer_sampler<Iterator, PropertyExtractor>::const_iterator::const_iterator(syncmer_sampler const& sampler, Iterator const& start)
    : parent_sampler(sampler), itr_start(start)
{
    find_first_syncmer();
}

template <class Iterator, typename PropertyExtractor>
typename syncmer_sampler<Iterator, PropertyExtractor>::const_iterator::value_type 
syncmer_sampler<Iterator, PropertyExtractor>::const_iterator::operator*() const
{
    assert(*itr_start);
    return optional_unwrap(*itr_start);
}

template <class Iterator, typename PropertyExtractor>
typename syncmer_sampler<Iterator, PropertyExtractor>::const_iterator const& 
syncmer_sampler<Iterator, PropertyExtractor>::const_iterator::operator++()
{
    ++itr_start;
    find_first_syncmer();
    return *this;
}

template <class Iterator, typename PropertyExtractor>
typename syncmer_sampler<Iterator, PropertyExtractor>::const_iterator 
syncmer_sampler<Iterator, PropertyExtractor>::const_iterator::operator++(int)
{
    auto current = *this;
    operator++();
    return current;
}

template <class Iterator, typename PropertyExtractor>
void
syncmer_sampler<Iterator, PropertyExtractor>::const_iterator::find_first_syncmer() noexcept
{
    std::size_t pos;
    while (itr_start != parent_sampler.itr_stop and 
           ((pos = parent_sampler.extor(*itr_start)) != parent_sampler.soffset and
             pos                                     != parent_sampler.eoffset)
        ) ++itr_start;
}

} // namespace sampler

#endif // SYNCMER_SAMPLER_HPP